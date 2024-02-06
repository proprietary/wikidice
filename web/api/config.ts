let baseUrl = 'http://localhost:8080';
if (process.env.NODE_ENV === 'development') {
    baseUrl = 'http://localhost:8080';
} else {
    baseUrl = "https://api.wikidice.com";
}
export default baseUrl;