<?php

/**
 * Starts user session forcefully
 * This is useful in case of mobile apps where sessid is required for anonymous users
 *
 * @return
 *   The user session object
 */

function _commerce_services_connect() {
  global $user;
  
  // checks if session is already started
  // if not start a new session and send back session object
  if(!drupal_session_started()) {
    drupal_session_start();
    
    $return = new stdClass();
    $return->sessid = session_id();
    $return->user = $user;

    //services_remove_user_data($account);

    return $return;
  } else {
    $return = new stdClass();
    $return->sessid = session_id();
    $return->user = $user;

    //services_remove_user_data($account);

    return $return;
  }
}

/**
 * Deletes user session
 *
 * @return
 */

function _commerce_services_session_destroy() {
  return session_destroy();
}