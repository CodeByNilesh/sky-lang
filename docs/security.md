# Sky Language — Security System

## Overview

Sky has a built-in security engine that runs at the language level.
Security is not a library. It is part of the compiler and runtime.

### Built-in Protections

- Rate limiting to prevent request flooding
- SQL injection detection with pattern-based blocking and auto-ban
- XSS detection for script injection blocking
- Path traversal detection for directory escape blocking
- Command injection detection for shell command blocking
- Brute force protection with failed login tracking and auto-blacklist
- DDoS protection with shield mode and whitelist-only access
- Attacker fingerprinting to track and identify malicious actors

## Configuration Syntax

    security {
        rate_limit 100/min per ip

        on brute_force {
            blacklist ip for 24h
            log threat(ip, "brute force detected")
            alert admin
        }

        on sql_injection {
            block request
            blacklist ip for 72h
            log threat(ip, request.raw)
            fingerprint attacker
        }

        on xss {
            block request
            blacklist ip for 48h
            log threat(ip, "xss attempt")
        }

        on path_traversal {
            block request
            blacklist ip for 24h
            log threat(ip, request.path)
        }

        on command_injection {
            block request
            blacklist ip for 72h
            log threat(ip, request.body)
            fingerprint attacker
        }

        on ddos {
            enable shield_mode
            allow only whitelisted
            log threat(ip, "ddos pattern")
        }
    }

## How The Pipeline Works

Every incoming HTTP request passes through this pipeline:

    Request arrives
        |
    [1] Blacklist check       -> blocked?         -> 403 Forbidden
        |
    [2] Shield mode check     -> not whitelisted? -> 403 Forbidden
        |
    [3] Rate limit check      -> exceeded?        -> 429 Too Many Requests
        |
    [4] SQL injection scan    -> detected?        -> block and ban
        |
    [5] XSS scan              -> detected?        -> block and ban
        |
    [6] Path traversal scan   -> detected?        -> block and ban
        |
    [7] Command injection scan -> detected?       -> block and ban
        |
    Request allowed -> route handler executes

## Detection Patterns

### SQL Injection Detection

Patterns that trigger blocking:

- Tautologies such as OR 1=1
- UNION attacks such as UNION SELECT
- Destructive such as DROP TABLE and DELETE FROM
- Comment-based such as admin with double dash
- Time-based such as SLEEP and BENCHMARK and pg_sleep
- Data extraction such as information_schema and LOAD_FILE
- Encoding tricks such as hex encoding and CHAR and CONCAT

### XSS Detection

Patterns that trigger blocking:

- Script tags
- Event handlers such as onerror and onload and onclick
- Protocol handlers such as javascript colon
- DOM access such as document.cookie and document.write
- Code execution such as eval
- Embedding such as iframe and object and embed and svg

### Path Traversal Detection

- Dot-dot-slash sequences
- URL encoded variants
- Sensitive file access such as /etc/passwd
- Windows path variants

### Command Injection Detection

- Shell chaining with semicolons and ampersands and pipes
- Backtick execution
- Subshell execution
- Network tool execution such as wget and curl and nc
- Direct binary path access

## Rate Limiting

    rate_limit 100/min per ip
    rate_limit 1000/hour per ip
    rate_limit 10/sec per ip

When rate limit is exceeded:

1. Request is blocked with 429 status
2. IP is flagged as potential brute force
3. After repeated violations IP is auto-blacklisted

## Blacklisting

    blacklist ip for 24h
    blacklist ip for 72h
    blacklist ip for 0h

Blacklisted IPs:

- Receive 403 Forbidden for all requests
- Are automatically removed when ban expires
- Can be manually removed via admin API

## Shield Mode

When activated:

1. ALL non-whitelisted IPs are blocked
2. Only pre-approved IPs can access the server
3. Every blocked request is logged
4. Must be manually disabled when attack stops

Example:

    whitelist "192.168.1.100"
    whitelist "10.0.0.1"

    on ddos {
        enable shield_mode
        allow only whitelisted
    }

## Threat Logging

All detected threats are logged with:

- Timestamp
- Source IP address
- Threat type
- Request details
- Attacker fingerprint when available

Log format:

    [SKY THREAT 2024-01-15 14:23:01] SQL_INJECTION from 192.168.1.50
    [SKY THREAT 2024-01-15 14:23:05] BRUTE_FORCE from 10.0.0.99
    [SKY THREAT 2024-01-15 14:24:00] DDOS from 203.0.113.0

## Fingerprinting

When fingerprint attacker is used Sky creates a unique identifier based on:

- IP address
- User-Agent header
- Other request headers
- Request patterns

Format: SKY-A1B2C3D4 (8-character hex hash)

This allows tracking attackers even if they change IP addresses.

## Best Practices

1. Always enable rate limiting even for internal APIs
2. Use strict bans for injection attacks with 72h minimum
3. Enable shield mode for critical services during attacks
4. Log everything for forensic analysis
5. Whitelist admin IPs before enabling shield mode
6. Review threat logs daily to detect patterns early
7. Use fingerprinting for persistent attackers
