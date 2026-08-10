#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#define PORT 8080
#define BUFFER_SIZE 8192
const char *p;
void skip_spaces()
{
    while (*p == ' ')
        p++;
}
double parse_number()
{
    skip_spaces();
    char *end;
    double value = strtod(p, &end);
    if (p == end)
        return NAN;
    p = end;
    return value;
}
double parse_factor()
{
    skip_spaces();
    if (*p == '+')
    {
        p++;
        return parse_factor();
    }
    if (*p == '-')
    {
        p++;
        return -parse_factor();
    }
    return parse_number();
}
double parse_term()
{
    double result = parse_factor();
    if (isnan(result))
        return NAN;
    while (1)
    {
        skip_spaces();
        if (*p == '*')
        {
            p++;
            double value = parse_factor();
            if (isnan(value))
                return NAN;
            result *= value;
        }
        else if (*p == '/')
        {
            p++;
            double value = parse_factor();
            if (isnan(value) || value == 0)
                return NAN;
            result /= value;
        }
        else
        {
            break;
        }
    }
    return result;
}
double parse_expression()
{
    double result = parse_term();
    if (isnan(result))
        return NAN;
    while (1)
    {
        skip_spaces();
        if (*p == '+')
        {
            p++;
            double value = parse_term();
            if (isnan(value))
                return NAN;
            result += value;
        }
        else if (*p == '-')
        {
            p++;
            double value = parse_term();
            if (isnan(value))
                return NAN;
            result -= value;
        }
        else
        {
            break;
        }
    }
    return result;
}
int calculate(const char *expression, double *result)
{
    p = expression;
    *result = parse_expression();
    skip_spaces();
    if (isnan(*result))
        return 0;
    if (*p != '\0')
        return 0;
    return 1;
}
char *read_file(const char *filename, long *size)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
        return NULL;
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    rewind(file);
    char *buffer = malloc(*size + 1);
    if (!buffer)
    {
        fclose(file);
        return NULL;
    }
    fread(buffer, 1, *size, file);
    buffer[*size] = '\0';
    fclose(file);
    return buffer;
}
void send_response(SOCKET client, const char *content_type, const char *body)
{
    char header[512];
    int body_length = (int)strlen(body);
    sprintf(
        header,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n"
        "\r\n",
        content_type,
        body_length
    );
    send(client, header, strlen(header), 0);
    send(client, body, body_length, 0);
}
void handle_client(SOCKET client)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    int received = recv(
        client,
        buffer,
        sizeof(buffer) - 1,
        0
    );

    if (received <= 0)
        return;
    if (strncmp(buffer, "GET / ", 6) == 0)
    {
        long size;

        char *html = read_file("index.html", &size);

        if (html)
        {
            send_response(
                client,
                "text/html",
                html
            );

            free(html);
        }

        return;
    }
    if (strncmp(buffer, "GET /style.css", 14) == 0)
    {
        long size;
        char *css = read_file("style.css", &size);
        if (css)
        {
            send_response(
                client,
                "text/css",
                css
            );
            free(css);
        }
        return;
    }
    if (strncmp(buffer, "GET /script.js", 14) == 0)
    {
        long size;
        char *js = read_file("script.js", &size);
        if (js)
        {
            send_response(
                client,
                "application/javascript",
                js
            );

            free(js);
        }

        return;
    }
    if (strncmp(buffer, "POST /calculate", 15) == 0)
    {
        char *body = strstr(buffer, "\r\n\r\n");

        if (!body)
            return;

        body += 4;
        char expression[256];
        sscanf(body, "expression=%255s", expression);
        // URL decode 
        char decoded[256];
        int j = 0;
        for (int i = 0; expression[i] && j < 255; i++)
        {
            if (expression[i] == '%')
            {
                int value;
                sscanf(
                    expression + i + 1,
                    "%2x",
                    &value
                );
                decoded[j++] = value;
                i += 2;
            }
            else if (expression[i] == '+')
            {
                decoded[j++] = ' ';
            }
            else
            {
                decoded[j++] = expression[i];
            }
        }
        decoded[j] = '\0';
        double result;
        char response[256];
        if (calculate(decoded, &result))
        {
            sprintf(
                response,
                "{\"success\":true,\"result\":%.12g}",
                result
            );
        }
        else
        {
            sprintf(
                response,
                "{\"success\":false,\"result\":\"Error\"}"
            );
        }
        send_response(
            client,
            "application/json",
            response
        );
        return;
    }
}
int main()
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("Winsock initialization failed.\n");
        return 1;
    }
    SOCKET server = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );
    if (server == INVALID_SOCKET)
    {
        printf("Could not create socket.\n");
        WSACleanup();
        return 1;
    }
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons(PORT);
    if (bind(
        server,
        (struct sockaddr *)&address,
        sizeof(address)
    ) == SOCKET_ERROR)
    {
        printf("Could not bind to port %d.\n", PORT);
        closesocket(server);
        WSACleanup();
        return 1;
    }
    listen(server, 10);
    printf("Calculator server running at:\n");
    printf("http://127.0.0.1:%d\n", PORT);
    printf("\nPress Ctrl+C to stop.\n");
    // Open browser automatically 
    ShellExecute(
        NULL,
        "open",
        "http://127.0.0.1:8080",
        NULL,
        NULL,
        SW_SHOWNORMAL
    );
    while (1)
    {
        SOCKET client = accept(
            server,
            NULL,
            NULL
        );
        if (client != INVALID_SOCKET)
        {
            handle_client(client);
            closesocket(client);
        }
    }
    closesocket(server);
    WSACleanup();
    return 0;
}