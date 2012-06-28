<?php

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
function _commerce_services_customer_profile_create($values = array()) {
	return $values;
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