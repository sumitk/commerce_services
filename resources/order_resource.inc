<?php

/**
 * Creates a new Commerce Order with support for cascading entity creation of
 * associated Commerce Line Items.
 *
 * @param $values
 *   An array of values to merge into the new order object:
 *   - uid: the uid of the user who owns the order
 *   - status: the order status to initialize the new order to; defaults to the
 *     default status of the "In progress" order state
 *   - order_number: an alphanumeric order number; defaults to the order ID
 *   - mail: the primary contact e-mail for this order
 *   - hostname: the IP address of the customer who generated this order
 *   - data: an array of additional data to store with the order.
 * @param $product
 *   Array of Product id and quantities of first product of cart "product":{"id":"2","quantity":4}
 * @param $billing
 *   Optional array of billing fields
 *   Example "billing":{"uid":1,"address":{"name":"mikka","street1":"my one street","street2":"My two 	    street","city":"Afreenpurra","county":"mikooo","postal_code":"93311","country":"US"}}
 *
 * @return
 *   The newly created order or FALSE with an error message.
 *
 * @see commerce_line_item_create.inc
 */

function _commerce_services_order_create($product = array()) {
  global $user;

  // Try to load current user's cart order
  $order_id = commerce_cart_order_id($user->uid);
 
  // If $order_id returned is false, no previous order exists
  // create a new order for user
  
  if($order_id) {
    $order = commerce_order_load($order_id);
    
    if (!empty($product['id'])) {
      $line_item = commerce_cart_product_add_by_id($product['id'], $product['quantity'], FALSE, $user->uid);
      return commerce_order_load($line_item->order_id);
    } else {
      return $order;
    } 
    
    return services_error(t('Error in adding product to cart.', array('@name' => $status)), 406);
  } else {
    // if no cart order is registered with user
    // product array is not empty
    if (!empty($product['id'])) {
      // creating a new order
      $order = commerce_order_new($user->uid, 'cart', 'commerce_order');
      // Save the order to get an order ID.
      commerce_order_save($order);
      $line_item = commerce_cart_product_add_by_id($product['id'], $product['quantity'], FALSE, $user->uid);
      // returning newly created order
      return commerce_order_load($line_item->order_id);
    } else if (!empty($order->order_id)) {
      return services_error(t('Product is not sent with first order.', array('@name' => $status)), 406);
    } 
    
    return services_error(t('Error in adding product to cart.', array('@name' => $status)), 406);  
  }
}

/**
 * Adds a new Product to existing Order {"order_id":42,"product":{"id":"1","quantity":9}}
 *
 * @param $order_id
 *   Existing order id
 * @param $product
 *   An array of product id and quantity:
 *   - id: the product id of product
 *   - quantity: product quantity to be added in order
 *
 * @return
 *   The line item id of new line item.
 */

function _commerce_services_order_add_product($product = array()) {
  global $user;

  // If line items should be created, do so now.
  if (!empty($product['id'])) {
	  $quantity = $product['quantity'];
	  $product = commerce_product_load($product['id']);
	  $line_item = commerce_product_line_item_new($product, $quantity);
	  
	  // adding product to user cart
	  commerce_cart_product_add($user->uid, $line_item, FALSE);
  }
  
  // Return line item id of new line item
  return $line_item;
}

/**
 * Deletes a product line item from a cart order.
 *
 * @param $order_id
 *   Order id of user order.
 * @param $line_item_id
 *   The ID of the line item to delete from the order.
 *
 * @return
 *   The order with the matching product line item deleted from the line item
 *     reference field.
 */
function _commerce_services_order_line_item_delete($order_id, $line_item_id) {
  global $user;

  // Try to load current user's cart order
  $order_id = commerce_cart_order_id($user->uid);

  // If $order_id returned is false, no previous order exists
  
  if(!$order_id) {
    return services_error(t('No order registered with this user.', array('@name' => $status)), 406);
  } else {
    $order = commerce_order_load($order_id);
    // loading order
    $order = commerce_order_load($order_id);
    // deleting line item from order
    $order_new = commerce_cart_order_product_line_item_delete($order, $line_item_id, FALSE);
    // returning new order onject
    return $order_new;
  }
   
}

/**
 * Edits quantity field of line item in product order 
 *
 * @param $order_id
 *   Order id of user order.
 * @param $line_item_id
 *   The ID of the line item to delete from the order.
 * @param $quantity
 *   New quantity for line item product.
 *
 * @return
 *   TRUE: if success in editing quantity. FALSE: if line item does not exist
 *     reference field.
 */
function _commerce_services_order_line_item_quantity_edit($line_item_id, $quantity) {
  global $user;

  // Try to load current user's cart order
  $order_id = commerce_cart_order_id($user->uid);

  // If $order_id returned is false, no previous order exists
  
  if(!$order_id) {
    return services_error(t('No order registered with this user.', array('@name' => $status)), 406);
  } else {  
    $line_item = commerce_line_item_load($line_item_id);
    $line_item_wrapper = entity_metadata_wrapper('commerce_line_item', $line_item);
    $product_id = $line_item_wrapper->commerce_product->product_id->value();

    // loading order
    $order = commerce_order_load($order_id);

    // deleting line item from order
    $order_new = commerce_cart_order_product_line_item_delete($order, $line_item_id, FALSE);

    if (!empty($product_id)) {
  	  $product = commerce_product_load($product_id);
  	  // creating new line item
  	  $line_item_new = commerce_product_line_item_new($product, $quantity);
  	  // adding updated line item to user cart
  	  return commerce_cart_product_add($user->uid, $line_item_new, FALSE);
    } else {
      return services_error(t('Error updating line item in cart.', array('@name' => $status)), 406);
    }
    return services_error(t('Error in call.', array('@name' => $status)), 406);
  }
}

/**
 * Deletes the Commerce Order with the specified ID.
 *
 * @param $order_id
 *   The ID of the order to load.
 *
 * @return
 *   TRUE on success or a 404 if it does not exist.
 */
function _commerce_services_order_delete($order_id) {
  // First attempt to load the specified order.
  if (!($order = commerce_order_load($order_id))) {
    return services_error(t('Order @order_id does not exist.', array('@order_id' => $order_id)), 404);
  }

  // Return the result of the delete call.
  return commerce_order_delete($order_id);
}

/**
 * Empty cart Order with specified ID.
 *
 * @param $order_id
 *   The ID of the order to load.
 *
 * @return
 *   TRUE on success or a 404 if it does not exist.
 */
function _commerce_services_cart_order_empty($order_id) {
  // First attempt to load the specified order.
  if (!($order = commerce_order_load($order_id))) {
    return services_error(t('Order @order_id does not exist.', array('@order_id' => $order_id)), 404);
  }

  // Return the result of the delete call.
  return commerce_cart_order_empty($order);
}

/**
 * Returns an array of Commerce Order arrays.
 *
 * An example request might look like:
 *
 * https://www.example.com/commerce-api/commerce_order?parameters[uid]=1
 *
 * This would return an array of orders for user 1.
 *
 * @param $parameters
 *   An array containing fields and values used to build a sql WHERE clause
 *   indicating what order should be included.
 * @param $page
 *   Page number of results to return (in pages of 30).
 *
 * @return
 *   An array of order arrays.
 */
function _commerce_services_order_index($parameters, $page) {
  $query = new EntityFieldQuery();
  $query
    ->addTag('commerce_order_access')
    ->entityCondition('entity_type', 'commerce_order')
    ->fieldCondition('external_uid', 'value', $parameters['uid'])
    ->fieldCondition('application', 'value', $parameters['application'])
    ->propertyOrderBy('order_id', 'DESC')
    ->range($page * 30, 30);

  // Loop over the results and build an array of order arrays.
  $result = $query->execute();
  $order_arrays = array();
  if (!empty($result['commerce_order'])) {
    $orders = entity_load('commerce_order', array_keys($result['commerce_order']));
    foreach ($orders as $order) {
      $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
      $order_arrays[] = commerce_services_object_to_array('commerce_order', $order_wrapper);
    }
  }
  return $order_arrays;
}

/**
 * Retrieves a Commerce Order with the specified ID.
 *
 * @param $order_id
 *   The ID of the order to load.
 *
 * @return
 *   The order if it could be found or a 404 if it does not exist.
 */
function _commerce_services_order_load($order_id) {
  // First attempt to load the specified order.
  if (!($order = commerce_order_load($order_id))) {
    return services_error(t('Order @order_id does not exist.', array('@order_id' => $order_id)), 404);
  }

  // Return the order as an array.
  //$order_wrapper = entity_metadata_wrapper('commerce_order', $order);
  return $order;//commerce_services_object_to_array('commerce_order', $order_wrapper);
}

/**
 * Updates the specified Commerce Order with information in the values array.
 *
 * @param $order_id
 *   The ID of the order to update.
 * @param $values
 *   An array of values to merge into the loaded order object. See comments for
 *   the commerce_order_create plugin for appropriate values.
 *
 * @return
 *   The saved order.
 */
function _commerce_services_order_save($order_id, $values) {
  $update_status = FALSE;

  // First attempt to load the specified order.
  if (!($order = commerce_order_load($order_id))) {
    return services_error(t('Order @order_id does not exist.', array('@order_id' => $order_id)), 404);
  }

  // If the status needs to change, use the actual API function.
  if (!empty($values['status']) && $values['status'] != $order->status) {
    $update_status = TRUE;
  }

  // Merge in the specified values.
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
  commerce_services_merge_values_to_object($values, 'commerce_order', $order_wrapper, array('status', 'commerce_order_total'));

  // Perform the actual order status update after merging in values.
  if ($update_status) {
    $order = commerce_order_status_update($order, $values['status'], TRUE);
  }

  // Save and return the order as an array.
  commerce_order_save($order);

  return commerce_services_object_to_array('commerce_order', $order_wrapper);
}

/**
 * Creates a new Commerce Customer Profile.
 *
 * @param $values
 *   An array of values to merge into the new profile object:
 *   - type: the of customer profile to create; defaults to billing
 *   - uid: the uid of the user who owns the customer profile
 *   - status: boolean indicating whether or not the profile is active; defaults
 *     to TRUE
 *   - address: an array of address data including:
 *     - name: the full name of the customer
 *     - street1: the first street line
 *     - street2: the second street line
 *     - city: the city
 *     - county: the county / state / province
 *     - postal_code: the postal code
 *     - country: the two character country code; defaults to site default
 *
 * @return
 *   The newly created customer profile or FALSE with an error message.
 */
function _commerce_services_order_customer_profile_create($values = array()) {
  // Return an error if an invalid type is specified.
  $type = empty($values['type']) ? 'billing' : $values['type'];

  if (!commerce_customer_profile_type_load($type)) {
    return services_error(t('Customer profile type @type does not exist.', array('@type' => $type)), 406);
  }

  // Return an error if a non-existent user is specified.
  $uid = !empty($values['uid']) ? $values['uid'] : 0;

  if (!commerce_services_user_exists($uid)) {
    return services_error(t('User @uid does not exist.', array('@uid' => $uid)), 406);
  }

  // Create the new customer profile.
  $profile = commerce_customer_profile_new($type, $uid);
  $wrapper = entity_metadata_wrapper('commerce_customer_profile', $profile);

  // Move unit price data to a key matching the field name.
  $address = array(
    'name' => !empty($values['address']['name']) ? $values['address']['name'] : '',
    'street1' => !empty($values['address']['street1']) ? $values['address']['street1'] : '',
    'street2' => !empty($values['address']['street2']) ? $values['address']['street2'] : '',
    'city' => !empty($values['address']['city']) ? $values['address']['city'] : '',
    'county' => !empty($values['address']['county']) ? $values['address']['county'] : '',
    'postal_code' => !empty($values['address']['postal_code']) ? $values['address']['postal_code'] : '',
    'country' => !empty($values['address']['country']) ? $values['address']['country'] :  variable_get('site_default_country', NULL),
  );

  $values['commerce_customer_address'] = $address;
  unset($values['address']);

  // Merge in the specified values.
  commerce_services_merge_values_to_object($values, 'commerce_customer_profile', $wrapper, array('type', 'uid'));

  // Save and return the customer profile.
  commerce_customer_profile_save($profile);

  return commerce_services_object_to_array('commerce_customer_profile', $wrapper);
}