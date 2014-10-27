import sys
import gtk

PALETTE_BORDER_WIDTH = 7

def bool(str):
    if str == '1' or str.lower() == 'true':
        return True
    return False

def dictify(list):
    return dict([(s.getAttribute('name'), s) for s in list])

def dictify_inorder(list):
    return [s.getAttribute('name') for s in list], dict([(s.getAttribute('name'), s) for s in list])

def warning(ctx, str):
    dialog = ctx.getWidget('WarningDialog')
    dialog.set_markup(str)
    status = dialog.run()
    dialog.hide()
    return status
    
def question(ctx, qst, *args, **kwargs):
    dialog = ctx.getWidget('QuestionDialog')
    dialog.set_markup(qst)
    status = dialog.run()
    dialog.hide()
    return status
    
def FancyExpander(name, choices, callback=None):
    palette = gtk.ToolPalette()
    group = gtk.ToolItemGroup(name)
    palette.add(group)
    palette.child_set_property(group, 'expand', False)
    palette.child_set_property(group, 'exclusive', False)
    group.set_collapsed(True)
    group.set_border_width(PALETTE_BORDER_WIDTH)    
    for choice in choices:
      item = gtk.ToolItem()
      group.insert(item, -1)
      group.child_set_property(item, 'new-row', True)
      group.child_set_property(item, 'expand', True)
      button = gtk.Button(label=choice)
      item.add(button)
      if callback:
          button.connect("clicked", callback, choice)
    palette.set_icon_size(gtk.ICON_SIZE_MENU)
    #palette.set_style(gtk.TOOLBAR_BOTH_HORIZ)
    palette.set_orientation(gtk.ORIENTATION_VERTICAL)
    palette.show_all()
    return palette
   