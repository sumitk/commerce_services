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
 * @param $line_items
 *   Optional array of line item values arrays used to create line items along
 *   with the new order.
 * @param $billing
 *   Optional array of billing fields
 *   Example "billing":{"uid":1,"address":{"name":"mikka","street1":"my one street","street2":"My two 	    street","city":"Afreenpurra","county":"mikooo","postal_code":"93311","country":"US"}}
 *
 * @return
 *   The newly created order or FALSE with an error message.
 *
 * @see commerce_line_item_create.inc
 */

function _commerce_services_order_create($values = array(), $line_items = array(), $billing = array()) {
  global $user;

  // Return an error if an invalid order status is specified.
  if (empty($values['status'])) {
    $order_state = commerce_order_state_load('in_progress');
    $status = $order_state['default_status'];
  }
  else {
    $status = $values['status'];

    if (!commerce_order_status_load($status)) {
      return services_error(t('Order status @name does not exist.', array('@name' => $status)), 406);
    }
  }

  // Create the new order.
  $order = commerce_order_new($user->uid, $status, 'commerce_order');
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);

  // Merge in the specified values.
  commerce_services_merge_values_to_object($values, 'commerce_order', $order_wrapper, array('uid', 'status', 'commerce_order_total'));

  // Save the order to get an order ID.
  commerce_order_save($order);
$order_wrapper = entity_metadata_wrapper('commerce_order', $order);
//return $order_wrapper->value();
  // Does the order need to be resaved? Used to avoid resaving twice.
  $needs_resave = FALSE;

  // If line items should be created, do so now.
  if (!empty($line_items)) {
    $line_item_ids = array();

    foreach ($line_items as $line_item_values) {
      $line_item = _commerce_services_order_line_item_create($order->order_id, $line_item_values);

      // If the line item creation failed, return the error message and exit.
      if (!$line_item || is_array($line_item)) {
        // First delete the order.
        commerce_order_delete($order->order_id);

        // Then delete any line items we've made already.
        foreach ($line_item_ids as $line_item_id) {
          commerce_line_item_delete($line_item_id);
        }

        // Now return the error message.
        if (!empty($line_item['error'])) {
          return services_error($line_item['error'], $line_item['status']);
        }
        else {
          return services_error(t('Could not create the requested line items.'), 406);
        }
      }

      // Add it to the order's line item reference value.
      $order_wrapper->commerce_line_items[] = $line_item;
      $line_item_ids[] = $line_item->line_item_id;
    }

    $needs_resave = TRUE;
  }

  // If a billing customer profile should be created, do so now.
  if (!empty($billing)) {
	  // defining address 
		$address = array(
	    'name' => !empty($billing['address']['name']) ? $billing['address']['name'] : '',
	    'street1' => !empty($billing['address']['street1']) ? $billing['address']['street1'] : '',
	    'street2' => !empty($billing['address']['street2']) ? $billing['address']['street2'] : '',
	    'city' => !empty($billing['address']['city']) ? $billing['address']['city'] : '',
	    'county' => !empty($billing['address']['county']) ? $billing['address']['county'] : '',
	    'postal_code' => !empty($billing['address']['postal_code']) ? $billing['address']['postal_code'] : '',
	    'country' => !empty($billing['address']['country']) ? $billing['address']['country'] :  variable_get('site_default_country', NULL),
	  );
	
    $profile = _commerce_services_order_customer_profile_create(array('type'=>'billing', 'uid'=>$user->uid,'address'=>$address));

    // If the profile creation failed, return the error message and exit.
    if (!$profile) {
      // First delete the order.
      commerce_order_delete($order->order_id);

      // Now return the error message.
      return services_error(t('Could not create a new billing profile with the given values.'), 406);
    }

    // Add the profile to the order.
		// commenting this because it is giving  invalid data value given
		// more details http://drupal.org/node/1151614
		//return $profile;
		
    //$order_wrapper->commerce_customer_billing->profile_id = $profile->profile_id;
    $needs_resave = TRUE;
  }

  // Resave the order if needed.
  if ($needs_resave) {
    commerce_order_save($order);
  }

  // Return the order converted to an array.
  return $order_wrapper->value();
}

/**
 * Creates an order line item from a line item values array.
 */
function _commerce_services_order_line_item_create($order_id, $values) {
  // Return an error if an invalid type is specified.
  $type = empty($values['type']) ? 'remote_product' : $values['type'];

  if (!commerce_line_item_type_load($type)) {
    return array('error' => t('Line item type @type does not exist.', array('@type' => $type)), 'status' => 406);
  }

  // Create the new line item.
  $line_item = commerce_line_item_new($type, $order_id);
  $line_item_wrapper = entity_metadata_wrapper('commerce_line_item', $line_item, array('bundle' => $type));

  // Move unit price data to a key matching the field name.
  $unit_price = array(
    'amount' => !empty($values['unit_price']['amount']) ? $values['unit_price']['amount'] : 0,
    'currency_code' => !empty($values['unit_price']['currency_code']) ? $values['unit_price']['currency_code'] : commerce_default_currency(),
    'data' => array(),
  );

  $values['commerce_unit_price'] = $unit_price;
  unset($values['unit_price']);

  // Move label data to a key matching the property name.
  if (!empty($values['label'])) {
    $values['line_item_label'] = $values['label'];
    unset($values['label']);
  }

  // Merge in the specified values.
  commerce_services_merge_values_to_object($values, 'commerce_line_item', $line_item_wrapper, array('type', 'order_id', 'commerce_total'));

  // Save the new line item.
  commerce_line_item_save($line_item);

  return $line_item;
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