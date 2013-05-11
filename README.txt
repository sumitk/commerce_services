Commerce Services resources (1.x)

Commerce Services defines Services module resources for Drupal Commerce entities
and systems, allowing any site to expose an API for displaying and manipulating
its eCommerce data remotely.

This module does not currently provide a comprehensive set of resources, because
the goal is to target "high priority" resources that are most likely to be used
in remote applications, like master / slave sites and mobile applications.

The following grid indicates what callbacks have been defined so far out of the
current target set of resources:

Resource         | Index | Retrieve | Create | Update | Delete
-----------------|-------|----------|--------|--------|--------
product-displays |   Y   |    Y     |   -    |   -    |   -
products         |   N   |    N     |   -    |   -    |   -
carts            |   N   |    -     |   -    |   -    |   -
orders           |   N   |    N     |   N    |   N    |   N

The additional relationships and targeted actions are slated for development:

product-displays: products
products: add to cart
carts: empty, convert
orders: payments

The orders resource will be a combined resource that is able to perform CRUD
operations on not just the order but also the entities an order may reference,
including customer profiles and line items.

For help understanding how REST APIs work, you might refer to:

- http://offers.apigee.com/api-design-ebook-rr/
- http://drupal.org/node/783254
- http://www.xfront.com/REST-Web-Services.html
- http://blog.steveklabnik.com/posts/2011-07-03-nobody-understands-rest-or-http
