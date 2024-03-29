/**
 * This file was auto-generated by openapi-typescript.
 * Do not make direct changes to the file.
 */


export interface paths {
  "/random/{category}": {
    /**
     * Pick a random article under a category
     * @description Get a random article from Wikipedia under a category
     */
    get: operations["get_random_article"];
  };
  "/random_with_derivation/{category}": {
    /**
     * Get a random article, but with details
     * @description Get a random article from Wikipedia under a category, but also return
     * the category path that was traversed to get to the article.
     */
    get: operations["get_random_article_with_derivation"];
  };
  "/autocomplete": {
    /**
     * Autocompletion search for category names
     * @description Prefix search for categories
     */
    get: operations["category_autocomplete"];
  };
}

export type webhooks = Record<string, never>;

export interface components {
  schemas: {
    /** Error */
    Error: {
      /**
       * error
       * @description Error message
       * @example Category not found
       */
      error?: string;
      /**
       * internal_code
       * @description Internal error code
       * @example 404
       */
      internal_code?: string | null;
    };
    /**
     * RandomArticleWithDerivation
     * @example {
     *   "derivation": [
     *     "Physics",
     *     "Subfields of physics",
     *     "Computational physics",
     *     "Lattice models"
     *   ],
     *   "article": 24535618
     * }
     */
    RandomArticleWithDerivation: {
      /**
       * article
       * @description Page ID
       */
      article?: number;
      /**
       * @description Derivation path to article (which categories were traversed to find
       * it)
       *
       * @example [
       *   "Physics",
       *   "Concepts in physics"
       * ]
       */
      derivation?: string[];
    };
  };
  responses: never;
  parameters: never;
  requestBodies: never;
  headers: never;
  pathItems: never;
}

export type $defs = Record<string, never>;

export type external = Record<string, never>;

export interface operations {

  /**
   * Pick a random article under a category
   * @description Get a random article from Wikipedia under a category
   */
  get_random_article: {
    parameters: {
      query?: {
        /** @description Maximum depth (recursive subcategories) to search for articles */
        depth?: number;
        /**
         * @description Wikipedia language name such as en, de, etc. For examples, see:
         * https://en.wikipedia.org/wiki/List_of_Wikipedias
         */
        language?: string;
      };
      path: {
        /** @description Category to search under (recursively, including subcategories) */
        category: string;
      };
    };
    responses: {
      /** @description Found */
      302: {
        headers: {
          /** @description Redirect location */
          Location?: string;
        };
        content: never;
      };
      /** @description Bad Request */
      400: {
        content: {
          "application/json": components["schemas"]["Error"];
        };
      };
      /** @description Not Found */
      404: {
        content: {
          "application/json": components["schemas"]["Error"];
        };
      };
      /** @description Internal Server Error */
      500: {
        content: {
          "application/json": components["schemas"]["Error"];
        };
      };
    };
  };
  /**
   * Get a random article, but with details
   * @description Get a random article from Wikipedia under a category, but also return
   * the category path that was traversed to get to the article.
   */
  get_random_article_with_derivation: {
    parameters: {
      query?: {
        /** @description Maximum depth (recursive subcategories) to search for articles */
        depth?: number;
        /**
         * @description Wikipedia language name such as en, de, etc. For examples, see:
         * https://en.wikipedia.org/wiki/List_of_Wikipedias
         */
        language?: string;
      };
      path: {
        /** @description Category to search under (recursively, including subcategories) */
        category: string;
      };
    };
    responses: {
      /** @description OK */
      200: {
        content: {
          "application/json": components["schemas"]["RandomArticleWithDerivation"];
        };
      };
      /** @description Bad Request */
      400: {
        content: {
          "application/json": components["schemas"]["Error"];
        };
      };
      /** @description Not Found */
      404: {
        content: {
          "application/json": components["schemas"]["Error"];
        };
      };
      /** @description Internal Server Error */
      500: {
        content: {
          "application/json": components["schemas"]["Error"];
        };
      };
    };
  };
  /**
   * Autocompletion search for category names
   * @description Prefix search for categories
   */
  category_autocomplete: {
    parameters: {
      query: {
        /** @description Prefix to search for */
        prefix: string;
        /**
         * @description Wikipedia language name such as en, de, etc. For examples, see:
         * https://en.wikipedia.org/wiki/List_of_Wikipedias
         */
        language?: string;
        /** @description Maximum number of results to return */
        limit?: number;
      };
    };
    responses: {
      /** @description OK */
      200: {
        content: {
          "application/json": string[];
        };
      };
      /** @description Bad Request */
      400: {
        content: {
          "application/json": components["schemas"]["Error"];
        };
      };
      /** @description Internal Server Error */
      500: {
        content: {
          "application/json": components["schemas"]["Error"];
        };
      };
    };
  };
}
