"""
Edit this file instead of the generated ones to change the behavior of the API.

Command to regenerate:

    mkdir /tmp/openapi_generated
    java -jar openapi-generator-cli-7.2.0.jar generate -g python-fastapi \
            -i openapi.yaml \
            -o /tmp/openapi_generated \
            --additional-properties fastapiImplementationPackage=openapi_server.impl,sourceFolder=
    mv /tmp/openapi_generated/openapi_server $(git rev-parse --show-toplevel)

"""

import os
from typing import List

from fastapi import Response

from openapi_server.apis.default_api_base import BaseDefaultApi

from openapi_server.models.error import Error

import build.pywikidice as pywikidice

WIKIDICE_DB = os.environ["WIKIDICE_DB"]

PYWIKIDICE_SESSION = pywikidice.Session(WIKIDICE_DB)


class DefaultApiImpl(BaseDefaultApi):
    def category_autocomplete(
        self,
        prefix: str,
        language: str,
    ) -> List[str]:
        """Prefix search for categories"""
        if language != "en":
            error = Error(
                code=400,
                message="Only English is supported at this time",
            )
            return Response(status_code=400, content=error)
        return PYWIKIDICE_SESSION.autocomplete_category_name(prefix)

    def get_random_article(
        self,
        category: str,
        language: str,
    ) -> None:
        """Get a random article from Wikipedia under a category"""
        page_id, ok = PYWIKIDICE_SESSION.pick_random_article(category)
        if not ok:
            return Response(status_code=404)
        wikipedia_url = f"https://{language}.wikipedia.org/?curid={page_id}"
        # Return result with a redirect
        return Response(status_code=302, headers={"Location": wikipedia_url})
