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
from openapi_server.models.random_article_with_derivation import RandomArticleWithDerivation


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
    limit: int = Query(10, description="Maximum number of results to return ", alias="limit", ge=1, le=100),
) -> List[str]:
    """Prefix search for categories"""
    return BaseDefaultApi.subclasses[0]().category_autocomplete(prefix, language, limit)


@router.get(
    "/random/{category}",
    responses={
        302: {"description": "Found"},
        400: {"model": Error, "description": "Bad Request"},
        404: {"model": Error, "description": "Not Found"},
        500: {"model": Error, "description": "Internal Server Error"},
    },
    tags=["default"],
    summary="Pick a random article under a category",
    response_model_by_alias=True,
)
async def get_random_article(
    category: str = Path(..., description="Category to search under (recursively, including subcategories)"),
    depth: int = Query(5, description="Maximum depth (recursive subcategories) to search for articles ", alias="depth", ge=0, le=100),
    language: str = Query('en', description="Wikipedia language name such as \&quot;en\&quot;, \&quot;de\&quot;, etc. For examples, see: https://en.wikipedia.org/wiki/List_of_Wikipedias ", alias="language"),
) -> None:
    """Get a random article from Wikipedia under a category"""
    return BaseDefaultApi.subclasses[0]().get_random_article(category, depth, language)


@router.get(
    "/random_with_derivation/{category}",
    responses={
        200: {"model": RandomArticleWithDerivation, "description": "OK"},
        404: {"model": Error, "description": "Not Found"},
        400: {"model": Error, "description": "Bad Request"},
        500: {"model": Error, "description": "Internal Server Error"},
    },
    tags=["default"],
    summary="Get a random article, but with details",
    response_model_by_alias=True,
)
async def get_random_article_with_derivation(
    category: str = Path(..., description="Category to search under (recursively, including subcategories)"),
    depth: int = Query(5, description="Maximum depth (recursive subcategories) to search for articles ", alias="depth", ge=0, le=100),
    language: str = Query('en', description="Wikipedia language name such as \&quot;en\&quot;, \&quot;de\&quot;, etc. For examples, see: https://en.wikipedia.org/wiki/List_of_Wikipedias ", alias="language"),
) -> RandomArticleWithDerivation:
    """Get a random article from Wikipedia under a category, but also return the category path that was traversed to get to the article. """
    return BaseDefaultApi.subclasses[0]().get_random_article_with_derivation(category, depth, language)
