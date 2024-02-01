"""
Edit this file instead of the generated ones to change the behavior of the API.

Command to regenerate:

    java -jar openapi-generator-cli-7.2.0.jar \
        generate -g python-fastapi \
        -i openapi.yaml \
        -o $(git rev-parse --show-toplevel) \
        --additional-properties fastapiImplementationPackage=openapi_server.impl,sourceFolder=

"""

import os
from typing import List

from fastapi import Response

from openapi_server.apis.default_api_base import BaseDefaultApi

from openapi_server.models.random_article_with_derivation import (
    RandomArticleWithDerivation,
)
from openapi_server.models.error import Error

import build.pywikidice as pywikidice

WIKIDICE_DB = os.environ["WIKIDICE_DB"]

PYWIKIDICE_SESSION = pywikidice.Session(WIKIDICE_DB)


class DefaultApiImpl(BaseDefaultApi):
    def category_autocomplete(
        self,
        prefix: str,
        language: str,
        limit: int,
    ) -> List[str]:
        """Prefix search for categories"""
        if language != "en":
            error = Error(
                code=400,
                message="Only English is supported at this time",
            )
            return Response(status_code=400, content=error)
        limit = max(1, min(limit, 100))
        prefix = prefix.replace(" ", "_")
        result = PYWIKIDICE_SESSION.autocomplete_category_name(prefix, limit)
        return [x.replace("_", " ") for x in result][:limit]

    def get_random_article(
        self,
        category: str,
        depth: int,
        language: str,
    ) -> None:
        """Get a random article from Wikipedia under a category"""
        if language != "en":
            return Response(
                content=Error(
                    code=400,
                    message="Only English is supported at this time",
                )
            )
        if depth > 100:
            return Response(
                content=Error(
                    code=400,
                    message="Depth cannot be greater than 100",
                ),
                status_code=400,
            )
        page_id, ok = PYWIKIDICE_SESSION.pick_random_article(category, depth)
        if not ok:
            return Response(status_code=404)
        wikipedia_url = f"https://{language}.wikipedia.org/?curid={page_id}"
        # Return result with a redirect
        return Response(status_code=302, headers={"Location": wikipedia_url})

    def get_random_article_with_derivation(
        self,
        category: str,
        depth: int,
        language: str,
    ) -> RandomArticleWithDerivation:
        """Get a random article from Wikipedia under a category, but also return
        the category path that was traversed to get to the article."""
        if language != "en":
            return Response(
                content=Error(
                    code=400,
                    message="Only English is supported at this time",
                )
            )
        if depth > 100:
            return Response(
                content=Error(
                    code=400,
                    message="Depth cannot be greater than 100",
                ),
                status_code=400,
            )
        category = category.replace(" ", "_")
        (
            page_id,
            ok,
            derivation,
        ) = PYWIKIDICE_SESSION.pick_random_article_and_show_derivation(category, depth)
        if not ok:
            return Response(status_code=404)
        # remove underscores from names
        for i in range(len(derivation)):
            derivation[i] = derivation[i].replace("_", " ")
        return RandomArticleWithDerivation(
            derivation=derivation,
            article=page_id,
        )
