# Security policy

## Reporting a vulnerability

Please report security problems privately, not in a public issue. Use GitHub's private reporting:
on the repository's **Security** tab, choose **Report a vulnerability**.

Please include the version or commit you tested, the compiler and flags, and a small program that
shows the problem: a crash, a data race reported by ThreadSanitizer, an error from AddressSanitizer,
an object handed out to two callers at once, and so on.

You can expect a reply within two weeks. Once a fix is ready, it is released and the report is
published as a GitHub security advisory, crediting you unless you'd rather not be named.

## Supported versions

Fixes are made on the latest release only.

## What counts as a vulnerability

- Data races, crashes, double deletes, or the same object being handed to two callers at once, when
  the library is used as documented.
- An object leaking out of a cache other than the one it was created by.

Not vulnerabilities, because the documentation makes them the caller's responsibility: data left
in a recycled object (use a reset policy that wipes it), and memory kept by the cache (call
`purge()`).
