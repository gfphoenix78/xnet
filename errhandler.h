//
// Created by Hao Wu on 3/1/20.
//

#ifndef XNET_ERRHANDLER_H
#define XNET_ERRHANDLER_H

struct ErrorContextFrame {
  struct ErrorContextFrame *prev;
  const char *message;
  void *extra;
  sigjmp_buf jmpbuf;
  int errcode;
  int depth;
};
extern struct ErrorContextFrame *g_ErrorContext_stack;

#define X_BEGIN_TRY()  \
    do { \
        ErrorContextFrame frame;
        frame.prev = g_ErrorContext_stask;
        if ((frame.errcode = sigsetjmp(frame.jmpbuf, 0)) == 0) \
        { \
            g_ErrorContext_stack = &frame

#define X_CATCH()  \
        } \
        else \
        { \
            g_ErrorContext_stack = frame.prev

#define X_END_TRY()  \
        } \
        g_ErrorContext_stack = frame.prev; \
      } while (0)

#define X_RETHROW(code) \
    do { \
      if (g_ErrorContext_stack) \
        siglongjmp(g_ErrorContext_stack->jmpbuf, (code)); \
      else { \
        abort(); \
      } \
    }while(0)

#endif //XNET_ERRHANDLER_H
