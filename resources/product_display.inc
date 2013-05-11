<?php

/**
 * @file
 * Contains the resource callbacks for product display nodes.
 */


/**
 * Returns a collection of product display nodes.
 *
 * @param $fields
 *   An array of field and property names to include in the returned product
 *   display node data. If empty, nodes will be returned as loaded with the
 *   exception of rdf_mapping data, which will only ever be included in an index
 *   if it is explicitly requested.
 * @param $expand_entities
 *   An integer depth to which entity reference field values should be expanded
 *   into full entities in a corresponding property on returned entities;
 *   defaults to 1.
 * @param $flatten_fields
 *   A string indicating whether or not field value arrays should be flattened;
 *   accepts true or false, defaults to true.
 * @param $filter
 *   An associative array of property names, single column field names, or
 *   multi-column field column names with their values to use to filter the
 *   result set of the index request.
 * @param $filter_op
 *   An associative array of field and property names with the operators to use
 *   when applying their filter conditions to the index request query.
 * @param $sort_by
 *   An array of database fields to sort the query by, with sort fields being
 *   valid properties, single column field names, or multi-column field column
 *   names for the matching entity type.
 * @param $sort_order
 *   The corresponding sort orders for the fields specified in the $sort_by
 *   array; one of either 'DESC' or 'ASC'.
 * @param $limit
 *   The number of product displays to retrieve.
 * @param $offset
 *   The zero-based index from which to start retrieving product displays.
 *
 * @return
 *   An array of product display nodes.
 */
function commerce_services_product_display_index($fields, $expand_entities, $flatten_fields, $filter, $filter_op, $sort_by, $sort_order, $limit, $offset) {
  // Build a query to load all accessible nodes that contain product reference
  // fields, whether those fields have data or not.
  $query = new EntityFieldQuery();
  $query
    ->addTag('node_access')
    ->addMetaData('op', 'view')
    ->addMetaData('base_table', 'node')
    ->entityCondition('entity_type', 'node')
    ->entityCondition('bundle', array_keys(commerce_product_reference_node_types()))
    ->range($offset, $limit);

  // Add the specified property and field condtions to the query.
  commerce_services_index_query_filter($query, 'node', $filter, $filter_op);

  // Add the specified sort conditions to the query.
  commerce_services_index_query_sort($query, 'node', $sort_by, $sort_order);

  // Execute the query and load the nodes it returns if any.
  $result = $query->execute();
  $nodes = array();

  if (!empty($result['node'])) {
    $nodes = node_load_multiple(array_keys($result['node']));

    foreach ($nodes as $nid => &$node) {
      // Add simplified fields to the entity object for certain field types.
      commerce_services_decorate_entity('node', $node);

      // Expand referenced entities into the full entity to the specified depth.
      if ($expand_entities > 0) {
        commerce_services_expand_entities('node', $node, $expand_entities, $flatten_fields == 'true');
      }

      // Flatten field value arrays if specified. This must be the last operation
      // performed as it breaks the standard field data model. An entity whose
      // fields have thus been flattened is no longer wrappable.
      if ($flatten_fields == 'true') {
        $node = clone($node);
        commerce_services_flatten_fields('node', $node);
      }

      // Unset the rdf_mapping key from any loaded node unless explicitly included
      // in the $fields array.
      if (!in_array('rdf_mapping', $fields)) {
        unset($node->rdf_mapping);
      }

      // Filter out the node properties based on the field list if given; always
      // remove rdf_mapping unless explicitly requested in the field list.
      if (!empty($fields)) {
        commerce_services_object_filter($node, $fields);
      }
    }
  }

  return $nodes;
}

/**
 * Determines access to perform product display operations.
 *
 * @param $op
 *   The operation being performed. One of 'view', 'update', 'create' or
 *   'delete'.
 * @param $args
 *   Resource arguments passed through from the original request.
 *
 * @return
 *   Boolean indicating whether or not the requested operation should be allowed.
 */
function commerce_services_product_display_access($op, $args) {
  // First load the node ID from the arguments if available.
  if ($op != 'create' && !empty($args[0])) {
    $node = node_load($args[0]);

    if (empty($node)) {
      return services_error(t('Product display not found'), 404);
    }
  }
  else {
    $node = NULL;
  }

  // If the request operation is a node create...
  if ($op == 'create') {
    // And the POSTed node object has a valid product display node type...
    if (!empty($args[0]->type) && in_array($args[0]->type, commerce_product_reference_node_types())) {
      // Return the user's access to create nodes of that type.
      if (!node_access($op, $args[0]->type)) {
        return services_error(t('Access to this operation not granted'), 401);
      }
      else {
        return TRUE;
      }
    }
    else {
      // Otherwise throw an error with an appropriate error message.
      return services_error(t('Valid product display node type required'), 400);
    }
  }

  // If the node could be loaded, return the node_access() value for the operation.
  if (!empty($node->nid)) {
    if (!node_access($op, $node)) {
      return services_error(t('Access to this operation not granted'), 401);
    }
    else {
      return TRUE;
    }
  }
  else {
    return services_error(t('Product display not found'), 404);
  }
}

/**
 * Returns a single product display node.
 *
 * @param $nid
 *   The node ID of the product display to retrieve.
 * @param $expand_entities
 *   An integer depth to which entity reference field values should be expanded
 *   into full entities in a corresponding property on returned entities;
 *   defaults to 1.
 * @param $flatten_fields
 *   A string indicating whether or not field value arrays should be flattened;
 *   accepts true or false, defaults to true.
 *
 * @return
 *   The requested product display node or FALSE if not found.
 */
function commerce_services_product_display_retrieve($nid, $expand_entities, $flatten_fields) {
  $node = node_load($nid);

  // If the node exists, determine its URI and add it to the node.
  if ($node) {
    $uri = entity_uri('node', $node);
    $node->path = url($uri['path'], array('absolute' => TRUE));

    // Unset the uri property if it's on the node to prevent recursion.
    unset($node->uri);

    // Add simplified fields to the entity object for certain field types.
    commerce_services_decorate_entity('node', $node);

    // Expand referenced entities into the full entity to the specified depth.
    if ($expand_entities > 0) {
      commerce_services_expand_entities('node', $node, $expand_entities, $flatten_fields == 'true');
    }

    // Flatten field value arrays if specified. This must be the last operation
    // performed as it breaks the standard field data model. An entity whose
    // fields have thus been flattened is no longer wrappable.
    if ($flatten_fields == 'true') {
      $node = clone($node);
      commerce_services_flatten_fields('node', $node);
    }
  }

  return $node;
}
