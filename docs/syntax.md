# Sky Language — Syntax Reference

## Variables

    let name = "Sky"
    let age = 25
    let pi = 3.14
    let active = true
    let items = [1, 2, 3]
    let user = {"key": "val"}

## Functions

    fn add(a int, b int) int {
        return a + b
    }

    fn greet(name string) {
        print("Hello " + name)
    }

## Classes

    class User {
        name string
        email string

        fn create(name string, email string) User {
            return User{name, email}
        }

        fn display(self) {
            print(self.name)
        }
    }

## Control Flow

    if condition {
        // then
    } else {
        // else
    }

    for i in 0..10 {
        // range loop
    }

    for item in collection {
        // iterator loop
    }

    while condition {
        // loop
    }

## Server and Routes

    server api on 8080 {
        route GET "/path" {
            respond 200 json({"key": "value"})
        }

        route POST "/path" {
            let body = request.body
            respond 201 json(body)
        }

        route GET "/auth" [auth] {
            respond 200 json({"msg": "protected"})
        }
    }

## Security

    security {
        rate_limit 100/min per ip

        on brute_force {
            blacklist ip for 24h
        }

        on sql_injection {
            block request
            blacklist ip for 72h
        }
    }

## Imports

    import db
    import jwt
    import http
    import crypto
    import "./my_module"

## Async

    async fn fetch(url string) {
        let res = await http.get(url)
        return res.body
    }
