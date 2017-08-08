<?php
/**
 * @file
 * EMWrapper class.
 */

/**
 * Class EMWrapper.
 *
 * Wraps an EntityMetadataWrapper and exposes functions to use this wrapper
 * without exceptions being thrown when a value is missing somewhere.
 */
class EMWrapper {

  /**
   * Entity metadata wrapper wrapped.
   *
   * @var EntityMetadataWrapper
   */
  private $wrapper;

  /**
   * Whether to log / display errors and exceptions or fail silently.
   *
   * @var bool
   */
  private $debug = TRUE;

  /**
   * Constructor.
   *
   * @param string $type
   *   The type of entity to wrap.
   * @param object $entity
   *   The entity to wrap.
   */
  public function __construct($type, $entity) {
    $this->wrapper = entity_metadata_wrapper($type, $entity);
  }

  /**
   * Set the wrapper language.
   *
   * The language is automatically set by Drupal during a request
   * but it can be useful to force it to explicitly work on
   * a given language.
   *
   * @param string $lang_code
   *   The language code to set.
   *
   * @return $this
   */
  public function setLanguage($lang_code) {
    if (method_exists($this->wrapper, 'language')) {
      $this->wrapper->language($lang_code);
    }

    return $this;
  }

  /**
   * Get a field value.
   *
   * @param string $field
   *   The name of the field to get.
   * @param bool $helper
   *   Some helpers are used to get values for fields of a certain type. Set
   *   this parameter to false to get the value as returned by
   *   $wrapper->{$field}->value() without using these helpers.
   * @param bool $sanitize
   *   Sanitize the output value if set to TRUE. Default to TRUE.
   *
   * @return mixed
   *   The field value if it exists, else NULL. Will return an
   *   EMWrapper object if the return value is wrappable and $helper
   *   equals to FALSE.
   */
  public function get($field, $helper = TRUE, $sanitize = TRUE) {
    global $language;

    $result = NULL;
    try {
      // Check current locale language before, using default.
      $this->setLanguage($language->language);
      if (!isset($this->wrapper->{$field}) || $this->wrapper->{$field}->value() === NULL) {
        $this->setLanguage(LANGUAGE_NONE);
      }
      if (isset($this->wrapper->{$field}) && $this->wrapper->{$field}->value() != NULL) {
        $field_info = $this->wrapper->getPropertyInfo($field);
        $field_type = (isset($field_info['type'])) ? $field_info['type'] : '';

        if ($helper) {
          // Try to use a helper.
          $elements = array();
          preg_match("/list<(.*)>/", $field_type, $elements);
          if (!empty($elements)) {
            // This is a list type field, call the helper on all the elements.
            foreach ($this->wrapper->{$field}->value() as $key => $value) {
              $potential_result = self::getFieldOfType($elements[1], $value, $sanitize);
              if ($potential_result) {
                $result[$key] = $potential_result;
              }
            }
          }
          else {
            // This is a single valued field.
            $result = self::getFieldOfType($field_type, $this->wrapper->{$field}->value(), $sanitize);
          }
        }

        if (!$result) {
          // No specific field management found, fall back to default behaviour.
          $result = $this->wrapper->{$field}->value(array('sanitize' => $sanitize));
        }
      }
    }
    catch (Exception $e) {
      if ($this->debug) {
        self::logError($e->getMessage() . ' : ' . $e->getTraceAsString());
      }
    }

    return $result;
  }

  /**
   * Set a field's value.
   *
   * TODO: manage multi valued fields an use helpers like for get().
   * (http://drupal.stackexchange.com/questions/50358/save-a-new-value-with-entity-metadata-wrapper-to-an-entity-field-which-is-an-arr).
   *
   * @param string $field
   *   The field to set.
   * @param mixed $value
   *   The value to set.
   */
  public function set($field, $value) {
    if (isset($this->wrapper->{$field})) {
      $this->wrapper->{$field}->set($value);
    }
  }

  /**
   * Return the wrapped DrupalWrapper.
   *
   * @return EntityMetadataWrapper
   *   The wrapper.
   */
  public function getWrapper() {
    return $this->wrapper;
  }

  /**
   * Return the original entity.
   *
   * @return Entity
   *   The original wrapped entity.
   */
  public function getOriginalEntity() {
    return $this->wrapper->value();
  }

  /**
   * Save the wrapper.
   */
  public function save() {
    $this->wrapper->save();
  }

  /**
   * Get all fields values.
   *
   * Useful to display all fields and properties available in a EMWrapper.
   *
   * @param bool $unfold
   *   If true, display values of sub XpWrappers. Default to FALSE for
   *   performance reasons.
   *
   * @return array
   *   All fields values indexed by field name.
   */
  public function getValues($unfold = FALSE) {
    $result = array();

    $fields = $this->wrapper->getPropertyInfo();

    foreach ($fields as $field => $info) {
      $value = $this->get($field);
      if (is_array($value)) {
        foreach ($value as $key => $item) {
          if ($unfold && $item instanceof EMWrapper) {
            $value[$key] = $item->getValues($unfold);
          }
        }
      }
      else {
        if ($unfold && $value instanceof EMWrapper) {
          $value = $value->getValues($unfold);
        }
      }

      $result[$field] = $value;
    }

    return $result;
  }

  /**
   * Getter helper for specials fields.
   *
   * @param string $field_type
   *   The special type of fields to get.
   * @param mixed $field_value
   *   The field value returned by $this->wrapper->{field_name}->value().
   * @param bool $sanitize
   *   Sanitize the output value if set to TRUE. Default to TRUE.
   *
   * @return mixed
   *   The field value if the field is a special field, else FALSE.
   */
  private static function getFieldOfType($field_type, $field_value, $sanitize = TRUE) {
    switch ($field_type) {
      case 'text_long':
      case 'text_formatted':
        if ($sanitize && isset($field_value['safe_value'])) {
          $result = $field_value['safe_value'];
        }
        else {
          $result = $field_value['value'];
        }
        break;

      case 'comment':
      case 'field_collection_item':
      case 'node':
      case 'file':
      case 'taxonomy_term':
      case 'taxonomy_vocabulary':
      case 'user':
        $result = new EMWrapper($field_type, $field_value);
        break;

      default:
        // May be a custom entity.
        $entity = entity_get_info($field_type);
        if (isset($entity['token type'])) {
          $result = new EMWrapper($field_type, $field_value);
        }
        else {
          $result = $field_value;
        }
        break;
    }

    return $result;
  }

  /**
   * Log / display an error.
   *
   * @param string $message
   *   The message error.
   */
  private static function logError($message) {
    watchdog('em_wrapper', $message);
    error_log($message);
    if (module_exists('devel')) {
      dpm($message);
    }
  }

}
