# Sky Language — API Development Guide

## Quick Start

Create a file app.sky:

    server api on 3000 {
        route GET "/hello" {
            respond 200 json({"msg": "Hello from Sky!"})
        }
    }

Run it:

    sky serve app.sky

Your API is live at http://localhost:3000/hello

## Server Block

The server keyword creates an HTTP server:

    server myapi on 8080 {
        // routes go here
    }

## Route Methods

Sky supports all standard HTTP methods:

    route GET "/users" {
        // Read data
    }

    route POST "/users" {
        // Create data
    }

    route PUT "/users" {
        // Full update
    }

    route PATCH "/users" {
        // Partial update
    }

    route DELETE "/users" {
        // Remove data
    }

## Request Object

Inside any route handler the request object is available:

    route POST "/example" {
        let body    = request.body
        let headers = request.headers
        let path    = request.path
        let query   = request.query
        let method  = request.method
        let ip      = request.ip
        let raw     = request.raw
    }

With auth middleware:

    route GET "/profile" [auth] {
        let user = request.user
        let uid  = request.user.id
    }

## Response

JSON Response:

    respond 200 json({"status": "ok"})
    respond 200 json(users)

Text Response:

    respond 200 "OK"
    respond 201 "Created"

Error Response:

    respond 400 json({"error": "missing required fields"})
    respond 401 json({"error": "unauthorized"})
    respond 404 json({"error": "not found"})
    respond 500 json({"error": "internal server error"})

## Status Codes

| Code | Meaning                |
|------|------------------------|
| 200  | OK                     |
| 201  | Created                |
| 204  | No Content             |
| 301  | Moved Permanently      |
| 302  | Found (Redirect)       |
| 400  | Bad Request            |
| 401  | Unauthorized           |
| 403  | Forbidden              |
| 404  | Not Found              |
| 405  | Method Not Allowed     |
| 429  | Too Many Requests      |
| 500  | Internal Server Error  |

## Authentication

Setting Up JWT Auth:

    import jwt

    server api on 3000 {
        route POST "/login" {
            let body = request.body
            let user = db.find("users", body.email)

            if user == nil {
                respond 401 json({"error": "user not found"})
            }

            if verify(user.password, body.password) {
                let token = jwt.sign(user.id)
                respond 200 json({"token": token})
            }

            respond 401 json({"error": "invalid password"})
        }
    }

Protected Routes:

    route GET "/profile" [auth] {
        let user_id = request.user.id
        let profile = db.find("users", user_id)
        respond 200 json(profile)
    }

The client must send the JWT token in the Authorization header:

    Authorization: Bearer your-token-here

## Database Operations

Import:

    import db

Query (returns result set):

    let users = db.query("SELECT * FROM users")

Execute (no result set):

    db.execute("INSERT INTO users (name, email) VALUES ('John', 'john@mail.com')")

Find (convenience):

    let user = db.find("users", email)

## JWT Tokens

    import jwt

    let token = jwt.sign(user_id)
    let payload = jwt.verify(token)

## Async Operations

    import http

    async fn fetch_external(url string) {
        let res = await http.get(url)
        return res.body
    }

    server api on 3000 {
        route GET "/proxy" {
            let data = await fetch_external("https://api.example.com/data")
            respond 200 json(data)
        }
    }

## Error Handling

    server api on 3000 {
        route GET "/users" {
            let users = db.query("SELECT * FROM users")

            if users == nil {
                respond 500 json({"error": "database error"})
            }

            if len(users) == 0 {
                respond 404 json({"error": "no users found"})
            }

            respond 200 json(users)
        }
    }

## Testing Your API

Using curl:

    curl http://localhost:3000/health

    curl -X POST http://localhost:3000/login -H "Content-Type: application/json" -d "{\"email\":\"user@test.com\",\"password\":\"secret\"}"

    curl http://localhost:3000/dashboard -H "Authorization: Bearer your-token"

## CLI Commands

    sky serve app.sky
    sky serve app.sky --port 9000
