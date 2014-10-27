#ifndef _CR_BLIT_H_
#define _CR_BLIT_H_
 
#include "cr-item.h"
#include "cr-canvas.h"

G_BEGIN_DECLS

#define CR_TYPE_BLIT  (cr_blit_get_type())

#define CR_BLIT(obj) \
        (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        CR_TYPE_BLIT, CrBlit))

#define CR_BLIT_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST ((klass), \
        CR_TYPE_BLIT, CrBlitClass))

#define CR_IS_BLIT(obj) \
        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CR_TYPE_BLIT))

#define CR_IS_BLIT_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE ((klass), CR_TYPE_BLIT))

#define CR_BLIT_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        CR_TYPE_BLIT, CrBlitClass))

typedef struct _CrBlit CrBlit;
typedef struct _CrBlitClass CrBlitClass;

struct _CrBlit
{
        CrItem parent;
        GdkRegion *region;
        cairo_pattern_t *pattern;
        unsigned char *buffer;
        gint32 flags;
        int timer_id, display_width, display_height;
        double device_width, device_height, scale_factor;
        CrCanvas *canvas;
};

struct _CrBlitClass
{
        CrItemClass parent_class;
};

GType cr_blit_get_type(void);

CrItem *cr_blit_new(CrItem *parent, const gchar *first_arg_name, ...);

enum {
        CR_BLIT_CANVAS = 1 << 0,
        CR_BLIT_REBUILD_PATTERN = 1 << 1,
        CR_BLIT_TEST_IMAGE = 1 << 2,
        CR_BLIT_UPDATE_READY = 1 << 3
};

G_END_DECLS

#endif /* _CR_BLIT_H_ */
