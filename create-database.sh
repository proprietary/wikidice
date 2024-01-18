#!/usr/bin/env bash

curl -LO https://dumps.wikimedia.org/enwiki/latest/enwiki-latest-category.sql.gz
gunzip enwiki-latest-category.sql.gz
curl -LO https://dumps.wikimedia.org/enwiki/latest/enwiki-latest-categorylinks.sql.gz
gunzip enwiki-latest-categorylinks.sql.gz
curl -LO https://dumps.wikimedia.org/enwiki/latest/enwiki-latest-page.sql.gz
gunzip enwiki-latest-page.sql.gz

mkdir -p data/wikidice.db

./build/wikidice_builder \
    --category_dump enwiki-latest-category.sql \
    --categorylinks_dump enwiki-latest-categorylinks.sql \
    --page_dump enwiki-latest-page.sql \
    --db_path data/wikidice.db \
    --threads $(nproc)

# Optionally, you can remove the SQL dumps to save space:
# rm enwiki-*.sql
