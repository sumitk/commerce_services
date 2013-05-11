<?php

/**
 * @file
 * Contains the resource callbacks for products.
 */


/**
 * Returns a collection of products.
 *
 * @see commerce_services_index_entities()
 */
function commerce_services_product_index($fields, $expand_entities, $flatten_fields, $filter, $filter_op, $sort_by, $sort_order, $limit, $offset) {
  return commerce_services_index_entities('commerce_product', $fields, $expand_entities, $flatten_fields, $filter, $filter_op, $sort_by, $sort_order, $limit, $offset);
}

/**
 * Determines an API user's access to index products.
 */
function commerce_services_product_index_access() {
  // Because the entity access control system will filter any result sets,
  // there's no reason to limit a particular user's access.
  return TRUE;
}

/**
 * Returns a single product.
 *
 * @see commerce_services_retrieve_entity()
 */
function commerce_services_product_retrieve($product_id, $expand_entities, $flatten_fields) {
  return commerce_services_retrieve_entity('commerce_product', $product_id, $expand_entities, $flatten_fields);
}

/**
 * Determines an API user's access to retrieve a given product.
 *
 * @param $product_id
 *   The ID of the product to be retrieved.
 *
 * @return
 *   Boolean indicating the user's access to retrieve the product.
 */
function commerce_services_product_retrieve_access($product_id) {
  // Attempt to load the product.
  if ($product = commerce_product_load($product_id)) {
    // And perform the view access check.
    if (commerce_product_access('view', $product)) {
      return TRUE;
    }
    else {
      return services_error(t('Access to this operation not granted'), 401);
    }
  }
  else {
    return services_error(t('Product not found'), 404);
  }
}

/**
 * Creates a new product.
 */
function commerce_services_product_create($data, $flatten_fields) {
  // Ensure the request specifies a unique, valid SKU.
  if (isset($data['sku'])) {
    $data['sku'] = trim($data['sku']);
  }

  if (empty($data['sku'])) {
    return services_error(t('You must specify a sku value'), 400);
  }
  elseif (!commerce_product_validate_sku($data['sku'])) {
    return services_error(t('Invalid sku value given'), 400);
  }
  elseif (!commerce_product_validate_sku_unique($data['sku'], NULL)) {
    return services_error(t('You must specify a unique sku value'), 400);
  }

  // Ensure the request specifies a title.
  if (empty($data['title'])) {
    return services_error(t('You must specify a title value'), 400);
  }

  // Ensure the request specifies a commerce_price amount and currency_code.
  $amount = NULL;
  $currency_code = NULL;

  // If the commerce_price field is set in the data array, examine its values
  // for a price amount and currency code.
  if (!empty($data['commerce_price'])) {
    if ($flatten_fields == 'true') {
      if (isset($data['commerce_price']['amount']) && is_numeric($data['commerce_price']['amount'])) {
        $amount = $data['commerce_price']['amount'];
      }
      if (!empty($data['commerce_price']['currency_code']) && commerce_currency_load($data['commerce_price']['currency_code'])) {
        $currency_code = $data['commerce_price']['currency_code'];
      }
    }
    else {
      $langcode = key($data['commerce_price']);

      if (isset($data['commerce_price'][$langcode][0]['amount']) && is_numeric($data['commerce_price'][$langcode][0]['amount'])) {
        $amount = $data['commerce_price'][$langcode][0]['amount'];
      }
      if (!empty($data['commerce_price'][$langcode][0]['currency_code']) && commerce_currency_load($data['commerce_price'][$langcode][0]['currency_code'])) {
        $currency_code = $data['commerce_price'][$langcode][0]['currency_code'];
      }
    }
  }

  // If not, look also in the column specific properties.
  if (isset($data['commerce_price_amount']) && is_numeric($data['commerce_price_amount'])) {
    $amount = $data['commerce_price_amount'];
  }
  if (!empty($data['commerce_price_currency_code']) && commerce_currency_load($data['commerce_price_currency_code'])) {
    $currency_code = $data['commerce_price_currency_code'];
  }

  if (is_null($amount)) {
    return services_error(t('You must specify a numeric commerce_price_amount value'), 400);
  }
  elseif (is_null($currency_code)) {
    return services_error(t('You must specify a valid commerce_price_currency_code value'), 400);
  }

  // Create the new product.
  $product = commerce_product_new($data['type']);

  // Remove the type from the data array since it's been set.
  unset($data['type']);

  // Set the field and property data and save the product.
  commerce_services_set_field_values('commerce_product', $product, $data, $flatten_fields);
  commerce_product_save($product);

  // Add simplified fields to the product object for certain field types.
  commerce_services_decorate_entity('commerce_product', $product);

  // Flatten field value arrays if specified. This must be the last operation
  // performed as it breaks the standard field data model. An entity whose
  // fields have thus been flattened is no longer wrappable or writable.
  if ($flatten_fields == 'true') {
    $product = clone($product);
    commerce_services_flatten_fields('commerce_product', $product);
  }

  return $product;
}

/**
 * Determines an API user's access to create a new product.
 */
function commerce_services_product_create_access($data) {
  // Ensure the request specifies a valid product type.
  if (empty($data['type']) || !in_array($data['type'], array_keys(commerce_product_types()))) {
    return services_error(t('You must specify a valid product type'), 400);
  }

  // Check the user's access to create products of the specified type.
  if (commerce_product_access('create', commerce_product_new($data['type']))) {
    return TRUE;
  }
  else {
    return services_error(t('Access to this operation not granted'), 401);
  }
}

/**
 * Updates a product.
 *
 * @see commerce_services_update_entity()
 */
function commerce_services_product_update($product_id, $data, $flatten_fields) {
  return commerce_services_update_entity('commerce_product', $product_id, $data, $flatten_fields);
}

/**
 * Determines an API user's access to update a given product.
 *
 * @param $product_id
 *   The ID of the product to be updated.
 *
 * @return
 *   Boolean indicating the user's access to update the product.
 */
function commerce_services_product_update_access($product_id) {
  // Attempt to load the product.
  if ($product = commerce_product_load($product_id)) {
    // If the user has access to perform the operation...
    if (commerce_product_access('update', $product)) {
      return TRUE;
    }
    else {
      return services_error(t('Access to this operation not granted'), 401);
    }
  }
  else {
    return services_error(t('Product not found'), 404);
  }
}

/**
 * Deletes a product.
 *
 * @param $product_id
 *   The ID of the product to be deleted.
 */
function commerce_services_product_delete($product_id) {
  commerce_product_delete($product_id);
}

/**
 * Determines an API user's access to delete a given product.
 *
 * @param $product_id
 *   The ID of the product to be deleted.
 *
 * @return
 *   Boolean indicating the user's access to delete the product.
 */
function commerce_services_product_delete_access($product_id) {
  // Attempt to load the product.
  if ($product = commerce_product_load($product_id)) {
    // If the user has access to perform the operation...
    if (commerce_product_access('delete', $product)) {
      // And the product can safely be deleted...
      if (commerce_product_can_delete($product)) {
        return TRUE;
      }
      else {
        return services_error(t('Product cannot be deleted'), 403);
      }
    }
    else {
      return services_error(t('Access to this operation not granted'), 401);
    }
  }
  else {
    return services_error(t('Product not found'), 404);
  }
}
