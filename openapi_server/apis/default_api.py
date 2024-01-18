# coding: utf-8

from typing import Dict, List  # noqa: F401
import importlib
import pkgutil

from openapi_server.apis.default_api_base import BaseDefaultApi
import openapi_server.impl

from fastapi import (  # noqa: F401
    APIRouter,
    Body,
    Cookie,
    Depends,
    Form,
    Header,
    Path,
    Query,
    Response,
    Security,
    status,
)

from openapi_server.models.extra_models import TokenModel  # noqa: F401
from openapi_server.models.error import Error


router = APIRouter()

ns_pkg = openapi_server.impl
for _, name, _ in pkgutil.iter_modules(ns_pkg.__path__, ns_pkg.__name__ + "."):
    importlib.import_module(name)


@router.get(
    "/autocomplete",
    responses={
        200: {"model": List[str], "description": "OK"},
        400: {"model": Error, "description": "Bad Request"},
        500: {"model": Error, "description": "Internal Server Error"},
    },
    tags=["default"],
    summary="Autocompletion search for category names",
    response_model_by_alias=True,
)
async def category_autocomplete(
    prefix: str = Query(None, description="Prefix to search for", alias="prefix"),
    language: str = Query('en', description="Wikipedia language name such as \&quot;en\&quot;, \&quot;de\&quot;, etc. For examples, see: https://en.wikipedia.org/wiki/List_of_Wikipedias ", alias="language"),
) -> List[str]:
    """Prefix search for categories"""
    return BaseDefaultApi.subclasses[0]().category_autocomplete(prefix, language)


@router.get(
    "/random/{category}",
    responses={
        302: {"description": "Found"},
        400: {"model": Error, "description": "Bad Request"},
        500: {"model": Error, "description": "Internal Server Error"},
    },
    tags=["default"],
    summary="Get a random article",
    response_model_by_alias=True,
)
async def get_random_article(
    category: str = Path(description="Category to search under (recursively, including subcategories)"),
    language: str = Query('en', description="Wikipedia language name such as \&quot;en\&quot;, \&quot;de\&quot;, etc. For examples, see: https://en.wikipedia.org/wiki/List_of_Wikipedias ", alias="language"),
) -> None:
    """Get a random article from Wikipedia under a category"""
    return BaseDefaultApi.subclasses[0]().get_random_article(category, language)
