<?php

/**
 * @file
 * Ding bookmark pages.
 */

/**
 * Menu callback; Display user bookmark items.
 */
function ding_bookmark_user_bookmark_page($account) {
  $content_ids = flag_load('bookmark')->get_user_content_ids($account->uid);

  if ($content_ids) {
    $bookmark_form = drupal_get_form('ding_bookmark_remove_form', $content_ids);
    // render reservation buttons outside bookmark form, so it's valid XHTML
    $reservation_forms = ding_bookmark_reservation_forms($content_ids);
    drupal_add_js(drupal_get_path('module', 'ding_bookmark') . '/js/ding_bookmark.js');
    drupal_add_css(drupal_get_path('module', 'ding_bookmark') . '/css/ding_bookmark.css');
    if (!empty($bookmark_form)) {
      return $reservation_forms . drupal_render($bookmark_form);
    }
  }

  return ding_bookmark_user_bookmark_empty_page();
}

/**
 * Bookmark page empty text.
 */
function ding_bookmark_user_bookmark_empty_page() {
  return t('Bookmarks is empty');
}

/**
 * Bookmark manage form.
 */
function ding_bookmark_remove_form($form, &$form_state, $entities) {
  $item_options = array();
  foreach ($entities as $entity_id => $entity_type) {
    // Build item view.
    $item_view_build = ding_bookmark_item_view($entity_type, $entity_id);
    if ($item_view_build) {
      $item_options[$entity_id] = $entity_id;
      $form['#builded_entities'][$entity_id] = $item_view_build;
    }
  }

  $form['items'] = array(
    '#type' => 'checkboxes',
    '#options' => $item_options,
    '#required' => TRUE,
  );

  $form['actions'] = array(
    '#type' => 'actions',
    '#attributes' => array(
      'class' => array('d-reservations', 'ui-buttons-panel'),
    ),
  );

  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Delete selected'),
    '#attributes' => array(
      'class' => array('d-reservations-delete'),
    ),
  );

  return $form;
}

/**
 * Submit handler.
 */
function ding_bookmark_remove_form_submit($form, &$form_state) {
  // Filter selected items.
  $items_to_delete = array_filter($form_state['values']['items']);
  if ($items_to_delete) {
    // Unflag all selected items.
    $flag = flag_get_flag('bookmark');
    foreach ($items_to_delete as $item_wid) {
      $flag->flag('unflag', $item_wid);
    }
  }
}

/**
 * Theme form as a table.
 */
function theme_ding_bookmark_remove_form($variables) {
  $output = "";
  $form = $variables['form'];

  // Add 'select all' checkbox.
  drupal_add_js('misc/tableselect.js');
  $header = array(
    array('class' => array('select-all')),
    array(
      'data' => t('Title'),
      'class' => 'title',
    ),
    '',
  );

  // Add classes to table header columns.

  // Create table rows.
  $rows = array();
  if (!empty($form['items']['#options'])) {

    foreach (element_children($form['items']) as $key) {
      $item_wid = $key;
      // Clean checkbox
      unset($form['items'][$key]['#title']);

      // Create action buttons.
      $reservable = FALSE;
      if ($form['#builded_entities'][$item_wid]['#object']->ding_entity_type == 'ding_entity' && $form['#builded_entities'][$item_wid]['#object']->is('reservable')) {
        $reservable = TRUE;
      }

      $buttons = field_get_items('ting_object', $form['#builded_entities'][$item_wid]['#object'], 'ding_entity_buttons');
      $buttons_output = field_view_value('ting_object', $form['#builded_entities'][$item_wid]['#object'], 'ding_entity_buttons', $buttons);
      $buttons_build = ding_bookmark_filter_buttons($buttons_output, $item_wid, $reservable);
      $row_actions = '<div class="btn-group actions">' . drupal_render($buttons_build) . '</div>';
      unset($buttons, $buttons_output, $buttons_build);

      //Add row (checkbox, item, buttons).
      $rows[] = array(
        array(
          'data' => drupal_render($form['items'][$key]),
          'class' => 'checkbox',
        ),
        array(
          'data' => drupal_render($form['#builded_entities'][$item_wid]),
          'class' => 'object',
        ),
        array(
          'data' => $row_actions,
          'class' => 'buttons',
        ),
      );
    }
  }


  if (count($rows)) {

    $output .= theme('table__alternative', array(
      'header' => $header,
      'rows' => $rows,
      'attributes' => array('width' => 'auto'),
    ));

    // Render all remaining elements.
    $output .= drupal_render_children($form);
  }

  return $output;
}

function ding_bookmark_reservation_forms($content_ids) {
  $reservation_block = '';
  foreach ($content_ids as $entity_id => $entity_type) {
    // build item view.
    $item_view_build = ding_bookmark_item_view($entity_type, $entity_id);
    // reservation buttons
    if ($item_view_build['#object']->ding_entity_type == 'ding_entity' && $item_view_build['#object']->is('reservable')) {
      $reservation_form = array(module_invoke('ding_reservation', 'ding_entity_buttons', 'ding_entity', $item_view_build['#object'], 'ajax'));
      $reservation_form[0]['#id'] = 'ding-reservation-reserve-form-wid' . $entity_id;
      // @TODO Update this, so class can be change in the theme layer.
      $reservation_div = array(
        '#prefix' => '<div class="ding-entity-button-reserve">',
        '#suffix' => '</div>',
        '#type' => 'markup',
        '#markup' => '',
      );
    }
  }
  return $reservation_block;
}

/**
 * Bookmark dummy form.
 */
function ding_bookmark_dummy_form($form, &$form_state) {
  $form = array();
  $form['#attributes'] = array('style' => array('visibility:hidden;'));
  return $form;
}

/**
 * Filter and fix action buttons.
 *
 * @param array $buttons
 *   Item action buttons.
 * @param string $item_id
 *   Item ID.
 * @param bool $reservable
 *   Is item reservable?
 *
 * @return array
 *   Filtered and fixed buttons.
 */
function ding_bookmark_filter_buttons($buttons, $item_id, $reservable) {
  $keys = array_keys($buttons);
  foreach($keys as $i) {
    if (empty($buttons[$i]['#options']['attributes']['class'])) {
      continue;
    }

    $classes = $buttons[$i]['#options']['attributes']['class'];
    // Remove "Find it here" button.
    if (array_search('find-it-here', $classes) !== FALSE) {
      unset($buttons[$i]);
      continue;
    }
    // Remove "Show info" button.
    if (array_search('show-info', $classes) !== FALSE) {
      unset($buttons[$i]);
      continue;
    }
    // Remove "Hide info" button.
    if (array_search('hide-info', $classes) !== FALSE) {
      unset($buttons[$i]);
      continue;
    }
    // Hide "Reserve" button if it is not reservable.
    if (array_search('reservation', $classes) !== FALSE && $reservable == FALSE) {
      unset($buttons[$i]);
      continue;
    }

    // Change bookmark button.
    if (array_search('bookmark', $classes) !== FALSE) {
      // Create delete link.
      $flag = flag_get_flag('bookmark');
      $delete_link_query = drupal_get_destination() + array('token' => flag_get_token($item_id));
      $delete_link = array(
        '#theme' => 'link',
        '#text' => '<i class="icon-white icon-remove-sign"></i>&nbsp;' . t($flag->unflag_short),
        '#path' => 'flag/unflag/bookmark/' . $item_id,
        '#options' => array(
          'attributes' => array(
            'class' => array(
              'btn',
              'btn-artesis-turquoise',
              'bookmark',
            )
          ),
          'html' => TRUE,
          'query' => $delete_link_query,
        ),
      );

      $buttons[$i] = $delete_link;
      continue;
    }
  }

  return $buttons;
}
