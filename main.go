package main

import (
	"database/sql"
	"fmt"
	"strings"
	"strconv"
	"html/template"
	"net/http"
	"encoding/json"
	"time"
	"context"
	"os"
	"os/signal"
	"log"
	"embed"
	_ "github.com/go-sql-driver/mysql"
)

var db *sql.DB

func main() {
	dsn := os.Getenv("SO_LIBHACK_WIKIDICE__MYSQL_DSN")
	if len(dsn) == 0 {
		log.Fatal("Missing database url environment variable")
	}
	var err error
	db, err = sql.Open("mysql", dsn)
	if err != nil {
		panic(err)
	}
	defer db.Close()
	db.SetConnMaxLifetime(time.Minute * 10)
	db.SetMaxOpenConns(10)
	db.SetMaxIdleConns(10)

	ctx, stop := context.WithCancel(context.Background())
	defer stop()

	appSignal := make(chan os.Signal, 3)
	signal.Notify(appSignal, os.Interrupt)

	go func() {
		select {
		case <-appSignal:
			stop()
		}
	}()

	Ping(ctx)

	http.HandleFunc("/lookup", handleLookup)
	http.HandleFunc("/category-members", handleGetCategoryMembers)
	http.HandleFunc("/category-autocomplete", handleCategoryAutocomplete)
	http.Handle("/static/", http.StripPrefix("/static/", http.FileServer(http.FS(staticAssets))))
	http.HandleFunc("/wikidice", func (w http.ResponseWriter, r *http.Request) {
		fmt.Fprintf(w, `<!doctype html>
<html>
  <head>
	<meta charset="utf-8">
	<meta name="go-import" content="wikidice.libhack.so/wikidice git https://github.com/proprietary/wikidice">
  </head>
  <body>
  </body>
</html>`)
	});
	http.HandleFunc("/", handleIndex)

	server := &http.Server{Addr: "0.0.0.0:42011", Handler: http.DefaultServeMux}
	go func() {
		if err := server.ListenAndServe(); err != nil {
			log.Fatal(err)
		}
	}()

	// Wait for signal to shutdown server

	// capture signals
	stopCh := make(chan os.Signal, 1)
	signal.Notify(stopCh, os.Interrupt)

	// wait for SIGINT
	<-stopCh

	ctx, cancel := context.WithTimeout(context.Background(), 5 * time.Second)
	defer cancel()
	if err := server.Shutdown(ctx); err != nil {
		log.Fatal(err)
	}
}

func Ping(ctx context.Context) {
	ctx, cancel := context.WithTimeout(ctx, 1*time.Second)
	defer cancel()

	if err := db.PingContext(ctx); err != nil {
		log.Fatalf("unable to connect to database: %v", err)
	}
}

func Lookup(ctx context.Context, category string, levels int) (string, error) {
	ctx, cancel := context.WithTimeout(ctx, 10*time.Second)
	defer cancel()

	var query string = `
with recursive catmems as (
	select page_id, cat_id, cl_type, 0 as depth
	from page_cat_ids
	where cat_id = (select page_id from page where page_title = ? and page_namespace = 14)

	union all

	select pci.page_id, pci.cat_id, pci.cl_type, catmems.depth + 1
	from page_cat_ids pci
	inner join catmems on catmems.cl_type = 'subcat' and catmems.page_id = pci.cat_id
	where (catmems.depth + 1) < ?
) select distinct page.page_title
	from catmems
	inner join page on page.page_id = catmems.page_id
	where catmems.cl_type = 'page'
	order by rand()
	limit 1;`
	var pageTitle string
	err := db.QueryRowContext(ctx, query, category, levels).Scan(&pageTitle)
	if err != nil {
		return "", err
	}
	return pageTitle, nil
}

func GetCategoryMembers(ctx context.Context, category string, levels int) ([]uint64, error) {
	var query string = `
with recursive catmems as (
	select page_id, cat_id, cl_type, 0 as depth
	from page_cat_ids
	where cat_id = (select page_id from page where page_title = ? and page_namespace = 14)

	union all

	select pci.page_id, pci.cat_id, pci.cl_type, catmems.depth + 1
	from page_cat_ids pci
	inner join catmems on catmems.cl_type = 'subcat' and catmems.page_id = pci.cat_id
	where (catmems.depth + 1) < ?
) select distinct page_id
	from catmems
	where cl_type = 'page'`
	rows, err := db.QueryContext(ctx, query, category, levels)
	if err != nil {
		return nil, err
	}
	defer rows.Close()
	out := make([]uint64, 0, 1000)
	for rows.Next() {
		var pageId uint64
		if err := rows.Scan(&pageId); err != nil {
			return nil, err
		}
		out = append(out, pageId)
	}
	if err := rows.Err(); err != nil {
		return nil, err
	}
	return out, nil
}

func CategoryNameAutocomplete(ctx context.Context, categoryNameStem string) ([]string, error) {
	const maxCompletions int = 10
	var query string = `select page_title from page where page_namespace = 14 and page_title like ? limit ?`
	categoryNameStem += "%"
	rows, err := db.QueryContext(ctx, query, categoryNameStem, maxCompletions)
	if err != nil {
		return nil, err
	}
	defer rows.Close()
	out := make([]string, 0, maxCompletions)
	for rows.Next() {
		var pageTitle string
		if err := rows.Scan(&pageTitle); err != nil {
			return nil, err
		}
		out = append(out, pageTitle)
	}
	if err := rows.Err(); err != nil {
		return nil, err
	}
	return out, nil
}

func PageIdToPageTitle(ctx context.Context, pageId int) string {
	return ""
}

func handleLookup(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "", http.StatusMethodNotAllowed)
		return
	}
	category := r.URL.Query().Get("category")
	if category == "" {
		http.Error(w, "Missing category in query string", http.StatusBadRequest)
		return
	}
	category = cleanCategoryName(category)
	var levels int = 2
	levelsStr := r.URL.Query().Get("levels")
	if levelsStr != "" {
		levelsNum, err := strconv.Atoi(levelsStr)
		if err == nil {
			levels = levelsNum
			if levels > maxLevels {
				levels = maxLevels
			} else if levels < minLevels {
				levels = minLevels
			}
		}
	}

	ctx := context.Background()
	ctx, cancel := context.WithTimeout(ctx, 2*time.Second)
	var errored bool = false
	var pageTitle string
	go func(pageTitle *string) {
		defer cancel()
		var err error
		*pageTitle, err = Lookup(ctx, category, levels)
		if err != nil {
			errored = true
			switch err {
			case sql.ErrNoRows:
				http.Error(w, fmt.Sprintf("There is no category called \"%s\". Make sure there is a page on Wikipedia called Category:%s", category, category), http.StatusBadRequest)
			}
		}
	}(&pageTitle)
	select {
	case <-ctx.Done():
		switch ctx.Err() {
		case context.DeadlineExceeded:
			http.Error(w, "Lookup took too long. Try again.", http.StatusRequestTimeout)
		case context.Canceled:
		}
	}
	if (!errored) {
		w.Header().Set("Location", fmt.Sprintf("https://en.wikipedia.org/wiki/%s", pageTitle))
		w.WriteHeader(http.StatusFound)
	}
}

func handleGetCategoryMembers(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "", http.StatusMethodNotAllowed)
		return
	}
	category := r.URL.Query().Get("category")
	if category == "" {
		http.Error(w, "Missing category in query string", http.StatusBadRequest)
		return
	}
	var levels int = 2
	levelsStr := r.URL.Query().Get("levels")
	if levelsStr != "" {
		levelsNum, err := strconv.Atoi(levelsStr)
		if err == nil {
			levels = levelsNum
			if levels > maxLevels {
				levels = maxLevels
			} else if levels < minLevels {
				levels = minLevels
			}
		}
	}

	ctx := context.Background()
	ctx, cancel := context.WithTimeout(ctx, 2*time.Second)
	var pageIds []uint64
	go func(pageIds *[]uint64) {
		defer cancel()
		var err error
		*pageIds, err = GetCategoryMembers(ctx, category, levels)
		if err != nil {
			log.Printf("error looking up category members (category=%s, levels=%d): %v", category, levels, err)
			http.Error(w, "", http.StatusInternalServerError)
			return
		}
	}(&pageIds)
	select {
	case <-ctx.Done():
		switch ctx.Err() {
		case context.DeadlineExceeded:
			http.Error(w, "timed out", http.StatusRequestTimeout)
			log.Println("timed out")
		case context.Canceled:
		}
	}
	w.Header().Set("Content-Type", "application/json")
	if err := json.NewEncoder(w).Encode(&pageIds); err != nil {
		log.Println("error encoding category members array as JSON", err)
		http.Error(w, "", http.StatusInternalServerError)
	}
	w.WriteHeader(http.StatusOK)
}

/// Category input autocompletion on search.
func handleCategoryAutocomplete(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "only GET allowed", http.StatusMethodNotAllowed)
		return
	}
	categoryStem := r.URL.Query().Get("q")
	if categoryStem == "" {
		out := make([]string, 0)
		if err := json.NewEncoder(w).Encode(&out); err != nil {
			log.Println(err)
			http.Error(w, "", http.StatusInternalServerError)
		}
		return
	}
	categoryStem = cleanCategoryName(categoryStem)
	ctx := context.Background()
	// TODO: add timeout
	out, err := CategoryNameAutocomplete(ctx, categoryStem)
	if err != nil {
		log.Println(err)
		http.Error(w, "", http.StatusInternalServerError)
		return
	}
	if err := json.NewEncoder(w).Encode(&out); err != nil {
		log.Println(err)
		http.Error(w, "", http.StatusInternalServerError)
		return
	}
}

/// cleanCategoryName converts a user-generated category page name to one the database understands. It converts spaces to underscores.
func cleanCategoryName(category string) string {
	var clean strings.Builder
	var space rune = ' '
	for _, c := range category {
		if c == space {
			clean.WriteRune('_')
		} else {
			clean.WriteRune(c)
		}
	}
	return clean.String()
}

// TODO: category input validation: is there a category for this name?

//go:embed templates/index.html
var indexHtml string

func handleIndex(w http.ResponseWriter, r *http.Request) {
	tmpl := template.New("index")
	tmpl.Parse(indexHtml)
	input := struct{
		Category string
		Levels string
	}{
		Category: r.URL.Query().Get("category"),
		Levels: r.URL.Query().Get("levels"),
	};
	if err := tmpl.Execute(w, input); err != nil {
		log.Println(err)
		http.Error(w, "", http.StatusInternalServerError)
		return
	}
}

//go:embed public/*.js
//go:embed public/favicon.ico
//go:embed public/favicon.png
var staticAssets embed.FS

const (
	minLevels int = 1
	maxLevels int = 4
)
