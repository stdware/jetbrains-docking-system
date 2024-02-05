#ifndef JBDSGLOBAL_H
#define JBDSGLOBAL_H

#include <QtCore/QtGlobal>

#ifndef JBDS_EXPORT
#  ifdef JBDS_STATIC
#    define JBDS_EXPORT
#  else
#    ifdef JBDS_LIBRARY
#      define JBDS_EXPORT Q_DECL_EXPORT
#    else
#      define JBDS_EXPORT Q_DECL_IMPORT
#    endif
#  endif
#endif

#endif // JBDSGLOBAL_H
