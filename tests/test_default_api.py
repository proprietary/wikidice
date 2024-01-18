# coding: utf-8

from fastapi.testclient import TestClient


from openapi_server.models.error import Error  # noqa: F401


def test_category_autocomplete(client: TestClient):
    """Test case for category_autocomplete

    Autocompletion search for category names
    """
    params = [("prefix", 'prefix_example'),     ("language", 'en')]
    headers = {
    }
    response = client.request(
        "GET",
        "/autocomplete",
        headers=headers,
        params=params,
    )

    # uncomment below to assert the status code of the HTTP response
    #assert response.status_code == 200


def test_get_random_article(client: TestClient):
    """Test case for get_random_article

    Get a random article
    """
    params = [("language", 'en')]
    headers = {
    }
    response = client.request(
        "GET",
        "/random/{category}".format(category='category_example'),
        headers=headers,
        params=params,
    )

    # uncomment below to assert the status code of the HTTP response
    #assert response.status_code == 200

