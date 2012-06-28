<?php

/**
 * Retrieves a Commerce Cart Order for the specified user.
 *
 * A "cart order" is the user's current order in progress awaiting completion.
 * For the Commerce API Server, we've defined a new "In progress" state with a
 * corresponding status to use to avoid having to enable the Cart module.
 *
 * @param $uid
 *   The uid of the user whose order should be returned.
 *
 * @return
 *   The requested cart order or FALSE if the user does not have a cart yet.
 */
function _commerce_services_cart_order_retrieve($uid) {
  // Create an array of valid shopping cart order statuses.
  $status_ids = array_keys(commerce_order_statuses(array('cart' => TRUE)));

  // Look for the user's most recent shopping cart order, although they should
  // never really have more than one.
  $query = new EntityFieldQuery();

  $query
    ->entityCondition('entity_type', 'commerce_order')
    ->propertyCondition('uid', $uid)
    ->propertyCondition('status', $status_ids, 'IN')
    ->propertyOrderBy('order_id', 'DESC')
    ->range(0, 1);
  $result = $query->execute();

  // If a valid cart order ID exists for the user, return it now.
  if (!empty($result['commerce_order'])) {
    $ids = array_keys($result['commerce_order']);
    $cart_order_id = reset($ids);
    $order = commerce_order_load($cart_order_id);

    if (!empty($order)) {
      return $order;
    }
  }

  return FALSE;
}

/**
 * Empties the line items from the Commerce Cart Order of the specified user.
 *
 * A "cart order" is the user's current order in progress awaiting completion.
 * For the Commerce API Server, we've defined a new "In progress" state with a
 * corresponding status to use to avoid having to enable the Cart module.
 *
 * @param $uid
 *   The uid of the user whose order should be emptied.
 *
 * @return
 *   The emptied cart order or FALSE if the user does not have a cart yet.
 */
function _commerce_services_cart_order_empty($uid) {
  // Create an array of valid shopping cart order statuses.
  $status_ids = array_keys(commerce_order_statuses(array('cart' => TRUE)));

  // Look for the user's most recent shopping cart order, although they should
  // never really have more than one.
  $query = new EntityFieldQuery();
  $query
    ->entityCondition('entity_type', 'commerce_order')
    ->propertyCondition('uid', $uid)
    ->propertyCondition('status', $status_ids, 'IN')
    ->propertyOrderBy('order_id', 'DESC')
    ->range(0, 1);
  $result = $query->execute();

  // If a valid cart order ID exists for the user, delete all its line items now.
  if (!empty($result['commerce_order'])) {
    $ids = array_keys($result['commerce_order']);
    $cart_order_id = reset($ids);
    $order = commerce_order_load($cart_order_id);

    if (!empty($order)) {
      // Build an array of product line item IDs.
      $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
      $line_item_ids = array();

      foreach ($order_wrapper->commerce_line_items as $delta => $line_item_wrapper) {
        $line_item_ids[] = $line_item_wrapper->line_item_id->value();
      }

      // Remove those line items from the order.
      foreach ($line_item_ids as $line_item_id) {
        $order = _commerce_services_cart_order_line_item_delete($order, $line_item_id);
      }

      // Save and return the now empty order.
      commerce_order_save($order);
			
			return $order;
      //return commerce_services_object_to_array('commerce_order', $order_wrapper);
    }
  }

  return FALSE;
}

/**
 * Deletes a product line item from a cart order.
 *
 * @param $order
 *   The cart order to delete from.
 * @param $line_item_id
 *   The ID of the line item to delete from the order.
 *
 * @return
 *   The order with the matching product line item deleted from the line item
 *     reference field.
 */
function _commerce_services_cart_order_line_item_delete($order, $line_item_id) {
  $line_item = commerce_line_item_load($line_item_id);

  // Check to ensure the line item exists.
  if (!$line_item) {
    return $order;
  }

  // Remove the line item from the line item reference field.
  $order = commerce_entity_reference_delete('commerce_order', $order, 'commerce_line_items', $line_item_id);

  // Delete the actual line item.
  commerce_line_item_delete($line_item->line_item_id);

  return $order;
}