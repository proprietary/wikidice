import type { paths } from './v1';
import createClient from 'openapi-fetch';
import baseUrl from './config';

const { GET } = createClient<paths>({ baseUrl });

export async function search(query: string): Promise<Array<string>> {
    const { data, error, response } = await GET('/autocomplete', {
        params: {
            query: {
                prefix: query,
                limit: 10,
            },
        },
    });
    if (error != null) {
        switch (response.status) {
            case 404:
                return [];
            case 500:
                console.error('Internal server error', error);
                return [];
            case 400:
                console.error('Bad request', error);
                return [];
            default:
                console.error(`Unknown error: ${response.status}`, error);
                return [];
        }
    }
    if (data == null) {
        console.error('No data returned');
        return [];
    }
    return data;
}

export async function lookup(categoryName: string, depth: number = 5): Promise<[number, Array<string>] | null> {
    const { data, error, response } = await GET('/random_with_derivation/{category}', {
        params: {
            path: {
                category: categoryName,
            },
            query: {
                depth,
                language: 'en',
            },
        },
    });
    if (error != null) {
        switch (response.status) {
            case 404:
                return null;
            case 500:
                console.error('Internal server error', error);
                return null;
            case 400:
                console.error('Bad request', error);
                return null;
            default:
                console.error(`Unknown error: ${response.status}`, error);
                return null;
        }
    }
    if (data == null) {
        console.error('No data returned');
        return null;
    }
    let pageId: number = 0;
    let derivation: Array<string> = [];
    if (typeof data.article != 'undefined')
        pageId = data.article;
    else
        console.error('No article returned');
    if (typeof data.derivation != 'undefined')
        derivation = data.derivation;
    else
        console.error('No derivation returned');
    return [pageId, derivation];
}