# Presentation

This module aims to ease the use of entity_metadata_wrapper.
By using this module you will avoid all the fatal errors you get when using
entity_metadata_wrapper to get values from an entity, they will be logged instead.

By default the values you get from the EMWrapper get() function
will be the sanitized ones.

Works with all entities.

See code of EMWrapper class for all the functionality.


# Use
```php
$node = node_load(1);
$wrapper = new EMWrapper('node', $node);

// Simple text field.
$wrapper->get('title');

// Entity references return an EMWrapper.
$wrapper->get('author')->get('mail');

// Multivalued entity references return EMWrappers too.
$wrapper->get('field_tags')[2]->get('name');
```

# Debug
To easily display all values of an entity, just call `EMWrapper::getValues()`.
By default if displays only entity info of the first level, it you want to
display info for all nested entities, set the $unfold parameter to TRUE.

# TODO
- test the set() function and see if it would be interesting to set values
this way (as setting values with entity_metadata_wrapper is already a pain in
the ass).
