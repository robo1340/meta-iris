openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -sha256 -days 999999 -nodes -subj "/C=XX/ST=StateName/L=CityName/O=CompanyName/OU=CompanySectionName/CN=CommonNameOrHostname"
