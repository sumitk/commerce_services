<?php

/**
 * @file
 * Contains the resource callbacks for cart orders.
 */


/**
 * Returns a collection of cart orders for the current user.
 *
 * @see commerce_services_index_entities()
 */
function commerce_services_cart_index($fields, $expand_entities, $flatten_fields, $filter, $filter_op, $sort_by, $sort_order, $limit, $offset) {
  global $user;

  // For now, this resource only returns the current cart order of the given API
  // user. We can expand the collection to include all cart orders sorted so the
  // current order is first in the list at a later time.
  $uid = !empty($user->uid) ? $user->uid : 0;
  $order_id = commerce_cart_order_id($uid);

  if (!empty($order_id)) {
    $filter['order_id'] = $order_id;
    $filter_op['order_id'] = '=';
  }
  else {
    return array();
  }

  return commerce_services_index_entities('commerce_order', $fields, $expand_entities, $flatten_fields, $filter, $filter_op, $sort_by, $sort_order, $limit, $offset);
  //return commerce_services_retrieve_entity('commerce_order', $fields, $expand_entities, $flatten_fields, $filter, $filter_op, $sort_by, $sort_order, $limit, $offset);
  
}

/**
 * Determines an API user's access to index cart orders.
 */
function commerce_services_cart_index_access() {
  // Because the entity access control system will filter any result sets,
  // there's no reason to limit a particular user's access.
  return TRUE;
}

/**
 * Creates a new cart order for the current API user.
 */
function commerce_services_cart_create($data, $flatten_fields) {
  global $user;
  $uid = !empty($user->uid) ? $user->uid : 0;

  // Even though the data array could technically accept a uid value, since the
  // resource is designed to work for the current API user this value will be
  // ignored and result in an error.
  if (isset($data['uid'])) {
    return services_error(t('Creating a cart does not permit you to set a uid value'), 400);
  }

  // Create the new order with the customer's uid and the cart order status.
  // Instead of using commerce_cart_order_new() directly, its contents have been
  // copied here to allow for setting field and property data before saving so
  // any errors in the data prevent saving a new order.
  $order = commerce_order_new($uid, 'cart');
  $order->log = t('Created as a shopping cart order.');

  // Set the field and property data and save the new order.
  commerce_services_set_field_values('commerce_order', $order, $data, $flatten_fields);
  commerce_order_save($order);

  // Reset the cart cache
  commerce_cart_order_ids_reset();

  // If the user is not logged in, ensure the order ID is stored in the session.
  if (!$uid && empty($user->uid)) {
    commerce_cart_order_session_save($order->order_id);
  }

  // Add simplified fields to the order object for certain field types.
  commerce_services_decorate_entity('commerce_order', $order);

  // Flatten field value arrays if specified. This must be the last operation
  // performed as it breaks the standard field data model. An entity whose
  // fields have thus been flattened is no longer wrappable or writable.
  if ($flatten_fields == 'true') {
    $order = clone($order);
    commerce_services_flatten_fields('commerce_order', $order);
  }

  return $order;
}

/**
 * Determines an API user's access to create new cart orders.
 */
function commerce_services_cart_create_access() {
  return TRUE;
  // If the user has access to perform the operation...
  if (commerce_order_access('create')) {
    return TRUE;
  }
  else {
    return services_error(t('Access to this operation not granted'), 401);
  }
}
