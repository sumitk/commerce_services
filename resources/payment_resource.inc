<?php

/**
 * Creates a new Commerce Payment Transaction.
 *
 * @param $values
 *   An array of values to merge into the new payment transaction object:
 *   - uid: the uid of the user who owns the order
 *   - order_id: the ID of the order to which the line item belongs
 *   - payment_method: the payment method method_id for this transaction
 *   - instance_id: the payment method instance_id for this transaction
 *   - remote_id: the remote identifier for this transaction at the gateway
 *   - message: the human-readable message associated to this transaction
 *   - message_variables: an array of variables associated with the
 *     human-readable message
 *   - amount: the price amount in cents
 *   - currency_code: the three chracter code of the transaction currency
 *   - status: the Commerce payment status of the transaction; pending, success,
 *     or failure
 *   - remote_status: the status of the transaction at the payment gateway
 *   - payload: an array of data returned for this transaction from the gateway
 *   - data: an array of additional data to store with the payment transaction
 *
 * @return
 *   The newly created payment transaction or FALSE with an error message.
 */
function _commerce_services_payment_transaction_create($values = array()) {
  $method_id = !empty($values['payment_method']) ? $values['payment_method'] : '';

  // Return an error if a non-existent order is specified.
  $order_id = !empty($values['order_id']) ? $values['order_id'] : 0;

  if (!commerce_services_order_exists($order_id)) {
    return services_error(t('Order @order_id does not exist.', array('@order_id' => $order_id)), 406);
  }

  // Create the new payment transaction.
  $transaction = commerce_payment_transaction_new($method_id, $order_id);

  // Merge in the specified values.
  commerce_services_merge_values_to_object($values, 'commerce_payment_transaction', $transaction, array('payment_method', 'order_id'));

  // Save and return the payment transaction as an array.
  commerce_payment_transaction_save($transaction);

  return commerce_services_object_to_array('commerce_payment_transaction', $transaction);
}