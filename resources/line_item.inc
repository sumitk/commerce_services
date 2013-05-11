<?php

/**
 * @file
 * Contains the resource callbacks for orders.
 */


/**
 * Returns a collection of line items.
 *
 * @see commerce_services_index_entities()
 */
function commerce_services_line_item_index($fields, $expand_entities, $flatten_fields, $filter, $filter_op, $sort_by, $sort_order, $limit, $offset) {
  return commerce_services_index_entities('commerce_line_item', $fields, $expand_entities, $flatten_fields, $filter, $filter_op, $sort_by, $sort_order, $limit, $offset);
}

/**
 * Determines an API user's access to index line items.
 */
function commerce_services_line_item_index_access() {
  // Because the entity access control system will filter any result sets,
  // there's no reason to limit a particular user's access. Users with the
  // administer line item permission will have access to view any line item but
  // other users will only have access to line items on orders they can view.
  return TRUE;
}

/**
 * Returns a single line item.
 *
 * @see commerce_services_retrieve_entity()
 */
function commerce_services_line_item_retrieve($line_item_id, $expand_entities, $flatten_fields) {
  return commerce_services_retrieve_entity('commerce_line_item', $line_item_id, $expand_entities, $flatten_fields);
}

/**
 * Determines an API user's access to retrieve a given line item.
 *
 * @param $line_item_id
 *   The ID of the line item to be retrieved.
 *
 * @return
 *   Boolean indicating the user's access to retrieve the line item.
 */
function commerce_services_line_item_retrieve_access($line_item_id) {
  // Attempt to load the line item.
  if ($line_item = commerce_line_item_load($line_item_id)) {
    // And perform the view access check.
    if (commerce_line_item_access('view', $line_item)) {
      return TRUE;
    }
    else {
      return services_error(t('Access to this operation not granted'), 401);
    }
  }
  else {
    return services_error(t('Line item not found'), 404);
  }
}

/**
 * Creates a new line item on an order.
 */
function commerce_services_line_item_create($data, $flatten_fields) {
  // Ensure the create request specifies a valid line item type.
  if (empty($data['type']) || !in_array($data['type'], array_keys(commerce_line_item_types()))) {
    return services_error(t('You must specify a valid line item type'), 400);
  }

  // Default the quantity to 1.
  if (empty($data['quantity'])) {
    $data['quantity'] = 1;
  }

  // Create the new line item.
  $line_item = commerce_line_item_new($data['type'], $data['order_id']);

  // Remove the type and order_id from the data array since they've been set.
  unset($data['type'], $data['order_id']);

  // If the line item is a product line item, ensure a commerce_product value
  // was passed referencing a valid product ID and populate the line item.
  if (in_array($line_item->type, commerce_product_line_item_types())) {
    $product = NULL;

    if (!empty($data['commerce_product'])) {
      if ($flatten_fields == 'true') {
        $product = commerce_product_load($data['commerce_product']);
      }
      else {
        $product = commerce_product_load($data['commerce_product']['und'][0]);
      }
    }

    // Bail now if the product wasn't found.
    if (empty($product)) {
      return services_error(t('You must specify a valid commerce_product value'), 400);
    }

    // Populate the line item.
    commerce_product_line_item_populate($line_item, $product);
  }

  // Set the field and property data and save the line item.
  commerce_services_set_field_values('commerce_line_item', $line_item, $data, $flatten_fields);
  commerce_line_item_save($line_item);

  // Add the line item to its order.
  $order = commerce_order_load($line_item->order_id);
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
  $order_wrapper->commerce_line_items[] = $line_item;
  commerce_order_save($order);

  // Add simplified fields to the line item object for certain field types.
  commerce_services_decorate_entity('commerce_line_item', $line_item);

  // Flatten field value arrays if specified. This must be the last operation
  // performed as it breaks the standard field data model. An entity whose
  // fields have thus been flattened is no longer wrappable or writable.
  if ($flatten_fields == 'true') {
    $line_item = clone($line_item);
    commerce_services_flatten_fields('commerce_line_item', $line_item);
  }

  return $line_item;
}

/**
 * Determines an API user's access to create new line items.
 */
function commerce_services_line_item_create_access($data) {
  return TRUE;
  // Load the order from the data array to determine access to create line items
  // on it.
  $order = NULL;

  if (!empty($data['order_id'])) {
    $order = commerce_order_load($data['order_id']);
  }

  if (empty($order)) {
    return services_error(t('You must specify a valid order ID'), 400);
  }

  // If the user has access to update the order...
  if (commerce_order_access('update', $order)) {
    return TRUE;
  }
  else {
    return services_error(t('Access to this operation not granted'), 401);
  }
}

/**
 * Updates a line item.
 *
 * This callback does not use commerce_services_update_entity(), because we must
 * ensure that the unit price is rebased if its amount or currency code change
 * but the data array is not updated to include the requisite price component.
 */
function commerce_services_line_item_update($line_item_id, $data, $flatten_fields) {
  // Load the line item.
  $line_item = commerce_line_item_load($line_item_id);

  // Set field values using the data provided.
  commerce_services_set_field_values('commerce_line_item', $line_item, $data, $flatten_fields);

  // Loop over the update data to determine if the unit price amount or
  // currency_code is updated without the data array being updated to include
  // price component updates...
  $rebase = FALSE;

  foreach (array_keys($data) as $key) {
    if (in_array($key, array('commerce_unit_price_amount', 'commerce_unit_price_currency_code')) &&
      !in_array('commerce_unit_price_data', array(array_keys($data)))) {
      $rebase = TRUE;
    }
    elseif ($key == 'commerce_unit_price' && empty($data[$key]['data']['components'])) {
      $rebase = TRUE;
    }
  }

  // Rebase the unit price if necessary.
  if ($rebase) {
    commerce_line_item_rebase_unit_price($line_item);
  }
  
  // Save the updates to the line item.
  commerce_line_item_save($line_item);

  // Add simplified fields to the line item for certain field types.
  commerce_services_decorate_entity('commerce_line_item', $line_item);

  // Flatten field value arrays if specified. This must be the last operation
  // performed as it breaks the standard field data model. An entity whose
  // fields have thus been flattened is no longer wrappable or writable.
  if ($flatten_fields == 'true') {
    $line_item = clone($line_item);
    commerce_services_flatten_fields('commerce_line_item', $line_item);
  }

  // If the line item quantity or unit price was updated, load and save the
  // order to recalculate the order total.
  foreach (array_keys($data) as $key) {
    if (in_array($key, array('quantity', 'commerce_unit_price')) || strpos($key, 'commerce_unit_price_') === 0) {
      $order = commerce_order_load($line_item->order_id);
      commerce_order_save($order);
      break;
    }
  }

  return $line_item;
}

/**
 * Determines an API user's access to update a given line item.
 *
 * @param $line_item_id
 *   The ID of the line item to be updated.
 *
 * @return
 *   Boolean indicating the user's access to update the line item.
 */
function commerce_services_line_item_update_access($line_item_id) {
  return TRUE;
  // Attempt to load the line item.
  if ($line_item = commerce_line_item_load($line_item_id)) {
    // If the user has access to perform the operation...
    if (commerce_line_item_access('update', $line_item)) {
      return TRUE;
    }
    else {
      return services_error(t('Access to this operation not granted'), 401);
    }
  }
  else {
    return services_error(t('Line item not found'), 404);
  }
}

/**
 * Deletes a line item.
 *
 * @param $line_item_id
 *   The ID of the line item to be deleted.
 */
function commerce_services_line_item_delete($line_item_id) {
  commerce_line_item_delete($line_item_id);
}

/**
 * Determines an API user's access to delete a given line item.
 *
 * @param $line_item_id
 *   The ID of the line item to be deleted.
 *
 * @return
 *   Boolean indicating the user's access to delete the line item.
 */
function commerce_services_line_item_delete_access($line_item_id) {
  // Attempt to load the line item.
  if ($line_item = commerce_line_item_load($line_item_id)) {
    // If the user has access to perform the operation...
    if (commerce_line_item_access('delete', $line_item)) {
      return TRUE;
    }
    else {
      return services_error(t('Access to this operation not granted'), 401);
    }
  }
  else {
    return services_error(t('Line item not found'), 404);
  }
}
