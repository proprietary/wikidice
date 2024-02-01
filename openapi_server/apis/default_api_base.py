# coding: utf-8

from typing import ClassVar, Dict, List, Tuple  # noqa: F401

from openapi_server.models.error import Error
from openapi_server.models.random_article_with_derivation import (
    RandomArticleWithDerivation,
)


class BaseDefaultApi:
    subclasses: ClassVar[Tuple] = ()

    def __init_subclass__(cls, **kwargs):
        super().__init_subclass__(**kwargs)
        BaseDefaultApi.subclasses = BaseDefaultApi.subclasses + (cls,)

    def category_autocomplete(
        self,
        prefix: str,
        language: str,
        limit: int,
    ) -> List[str]:
        """Prefix search for categories"""
        ...

    def get_random_article(
        self,
        category: str,
        depth: int,
        language: str,
    ) -> None:
        """Get a random article from Wikipedia under a category"""
        ...

    def get_random_article_with_derivation(
        self,
        category: str,
        depth: int,
        language: str,
    ) -> RandomArticleWithDerivation:
        """Get a random article from Wikipedia under a category, but also return the category path that was traversed to get to the article."""
        ...
