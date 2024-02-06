let baseUrl = 'http://localhost:8080';
if (process.env.NODE_ENV === 'development') {
    baseUrl = 'http://localhost:8080';
} else {
    baseUrl = "https://wikidice.zelcon.net/api/";
}
export default baseUrl;