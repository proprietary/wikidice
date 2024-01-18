# coding: utf-8

from typing import ClassVar, Dict, List, Tuple  # noqa: F401

from openapi_server.models.error import Error


class BaseDefaultApi:
    subclasses: ClassVar[Tuple] = ()

    def __init_subclass__(cls, **kwargs):
        super().__init_subclass__(**kwargs)
        BaseDefaultApi.subclasses = BaseDefaultApi.subclasses + (cls,)
    def category_autocomplete(
        self,
        prefix: str,
        language: str,
    ) -> List[str]:
        """Prefix search for categories"""
        ...


    def get_random_article(
        self,
        category: str,
        language: str,
    ) -> None:
        """Get a random article from Wikipedia under a category"""
        ...
