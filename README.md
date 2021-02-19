# **php-webops-event**

## Requires

PHP_VERSION >= 8.0

## Description

The idea behind this module is to take the functions of [php-apm](https://pecl.php.net/package/APM) that Pantheon uses in it's webops environments and port those functions to a [PHP 8.0](https://php.net) clean extension.

When enabled, this module will take an error or thowable event happening in PHP8 and push debugging information about the event to a https REST endpoint.

## Credit Where Due

This module is based on the fantastic work of Davide Mendolia & Patrick Allaert who created the original PHP-APM module.

Thank you [Sara Golemon](https://github.com/sgolemon), for quite literally writing the book [tutorial](https://flylib.com/books/en/2.565.1/) on php internals.

This module is sponsored by and enabled in production on [PANTHEON](https://pantheon.io)&rsquo;s WEBOPS CMS hosting platform (this line to be vetted by product and legal before PECL extension is published).

[[Zeus logo]]

**"Pantheon powers over 300,000 sites and is trusted by thousands of marketing and development teams around the world. We’re just getting started."**

## Authors

[Tom Stovall](https://github.com/stovak) 2021-FEB
