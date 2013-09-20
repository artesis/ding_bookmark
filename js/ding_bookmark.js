(function($) {

  Drupal.behaviors.bookmarkSubmit = {
    attach:function(context, settings) {
      $('.ding-bookmark-reservation-button', context).click(function() {
        $('#ding-reservation-reserve-form-' + this.id + ' input[type=submit]').mousedown();
        return false;
      });
    }
  };

  // reload page after closing reservation pop-up
  Drupal.behaviors.reloadBookmarkOnPopupClose = {
    attach: function(context) {
      // Capture link clicks.
      $('body.page-user-bookmarks a.ui-dialog-titlebar-close').each( function(context) {
        // Unbind existing click behaviors.
        $(this).unbind('click');
        // Handle clicks.
        $(this).click( function(e) {
          // Remove the default click handler.
          e.preventDefault();
          location.reload();
          return false;
        });
      });
    }
  };

  init_behavior = function(ajax, response, status) {
    var entity_id = response.data;
    var forms = $('form');
    var regex = new RegExp(entity_id, 'g');

    // Loop through all forms on a page, deeper filtering comes next.
    forms.each(function() {
      form = $(this);

      // Wee seek for reservations forms, thus specific form whose item was clicked.
      if (form.attr('id').match(/flag-confirm/g) && form.attr('action').match(regex)) {
        // Dynamic form, so reattach the behaviors.
        Drupal.attachBehaviors(form);
        form.hide();
        form.find('.form-submit').mousedown();
      }
    });
  }

  Drupal.behaviors.ding_bookmark = {
    attach: function (context, settings) {
      Drupal.ajax.prototype.commands.init_behavior = init_behavior;
    }
  };
})(jQuery);
