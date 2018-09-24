/*
 * Copyright (c) 2012, 2016, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

#ifndef SHARE_VM_JVMCI_JVMCI_RUNTIME_HPP
#define SHARE_VM_JVMCI_JVMCI_RUNTIME_HPP

#include "interpreter/interpreter.hpp"
#include "memory/allocation.hpp"
#include "runtime/arguments.hpp"
#include "runtime/deoptimization.hpp"

#define JVMCI_ERROR(...)       \
  { JVMCIRuntime::fthrow_error(THREAD_AND_LOCATION, __VA_ARGS__); return; }

#define JVMCI_ERROR_(ret, ...) \
  { JVMCIRuntime::fthrow_error(THREAD_AND_LOCATION, __VA_ARGS__); return ret; }

#define JVMCI_ERROR_0(...)    JVMCI_ERROR_(0, __VA_ARGS__)
#define JVMCI_ERROR_NULL(...) JVMCI_ERROR_(NULL, __VA_ARGS__)
#define JVMCI_ERROR_OK(...)   JVMCI_ERROR_(JVMCIEnv::ok, __VA_ARGS__)
#define CHECK_OK              CHECK_(JVMCIEnv::ok)

class JVMCIRuntime: public AllStatic {
 public:
  // Constants describing whether JVMCI wants to be able to adjust the compilation
  // level selected for a method by the VM compilation policy and if so, based on
  // what information about the method being schedule for compilation.
  enum CompLevelAdjustment {
     none = 0,             // no adjustment
     by_holder = 1,        // adjust based on declaring class of method
     by_full_signature = 2 // adjust based on declaring class, name and signature of method
  };

 private:
  static jobject _HotSpotJVMCIRuntime_instance;
  static bool _HotSpotJVMCIRuntime_initialized;

  static CompLevelAdjustment _comp_level_adjustment;

  static bool _shutdown_called;

  static CompLevel adjust_comp_level_inner(methodHandle method, bool is_osr, CompLevel level, JavaThread* thread);

 public:

  /**
   * Ensures that the JVMCI class loader is initialized and the well known JVMCI classes are loaded.
   */
  static void ensure_jvmci_class_loader_is_initialized();

  static bool is_HotSpotJVMCIRuntime_initialized() {
    return _HotSpotJVMCIRuntime_initialized;
  }

  /**
   * Gets the singleton HotSpotJVMCIRuntime instance, initializing it if necessary
   */
  static Handle get_HotSpotJVMCIRuntime(TRAPS) {
    initialize_JVMCI(CHECK_(Handle()));
    return Handle(JNIHandles::resolve_non_null(_HotSpotJVMCIRuntime_instance));
  }

  static jobject get_HotSpotJVMCIRuntime_jobject(TRAPS) {
    initialize_JVMCI(CHECK_NULL);
    assert(_HotSpotJVMCIRuntime_initialized, "must be");
    return _HotSpotJVMCIRuntime_instance;
  }

  static Handle callStatic(const char* className, const char* methodName, const char* returnType, JavaCallArguments* args, TRAPS);

  /**
   * Trigger initialization of HotSpotJVMCIRuntime through JVMCI.getRuntime()
   */
  static void initialize_JVMCI(TRAPS);

  /**
   * Explicitly initialize HotSpotJVMCIRuntime itself
   */
  static void initialize_HotSpotJVMCIRuntime(TRAPS);

  static void metadata_do(void f(Metadata*));

  static void shutdown(TRAPS);

  static void bootstrap_finished(TRAPS);

  static bool shutdown_called() {
    return _shutdown_called;
  }

  /**
   * Lets JVMCI modify the compilation level currently selected for a method by
   * the VM compilation policy.
   *
   * @param method the method being scheduled for compilation
   * @param is_osr specifies if the compilation is an OSR compilation
   * @param level the compilation level currently selected by the VM compilation policy
   * @param thread the current thread
   * @return the compilation level to use for the compilation
   */
  static CompLevel adjust_comp_level(methodHandle method, bool is_osr, CompLevel level, JavaThread* thread);

  /**
   * Throws a JVMCIError with a formatted error message. Ideally we would use
   * a variation of Exceptions::fthrow that takes a class loader argument but alas,
   * no such variation exists.
   */
  static void fthrow_error(Thread* thread, const char* file, int line, const char* format, ...) ATTRIBUTE_PRINTF(4, 5);

  /**
   * Exits the VM due to an unexpected exception.
   */
  static void exit_on_pending_exception(Handle exception, const char* message);

#define CHECK_EXIT THREAD); \
  if (HAS_PENDING_EXCEPTION) { \
    char buf[256]; \
    jio_snprintf(buf, 256, "Uncaught exception at %s:%d", __FILE__, __LINE__); \
    JVMCIRuntime::exit_on_pending_exception(PENDING_EXCEPTION, buf); \
    return; \
  } \
  (void)(0

#define CHECK_EXIT_(result) THREAD); \
  if (HAS_PENDING_EXCEPTION) { \
    char buf[256]; \
    jio_snprintf(buf, 256, "Uncaught exception at %s:%d", __FILE__, __LINE__); \
    JVMCIRuntime::exit_on_pending_exception(PENDING_EXCEPTION, buf); \
    return result; \
  } \
  (void)(0

  /**
   * Same as SystemDictionary::resolve_or_null but uses the JVMCI loader.
   */
  static Klass* resolve_or_null(Symbol* name, TRAPS);

  /**
   * Same as SystemDictionary::resolve_or_fail but uses the JVMCI loader.
   */
  static Klass* resolve_or_fail(Symbol* name, TRAPS);

  static BasicType kindToBasicType(Handle kind, TRAPS);

  static void new_instance_common(JavaThread* thread, Klass* klass, bool null_on_fail);
  static void new_array_common(JavaThread* thread, Klass* klass, jint length, bool null_on_fail);
  static void new_multi_array_common(JavaThread* thread, Klass* klass, int rank, jint* dims, bool null_on_fail);
  static void dynamic_new_array_common(JavaThread* thread, oopDesc* element_mirror, jint length, bool null_on_fail);
  static void dynamic_new_instance_common(JavaThread* thread, oopDesc* type_mirror, bool null_on_fail);

  // The following routines are called from compiled JVMCI code

  // When allocation fails, these stubs:
  // 1. Exercise -XX:+HeapDumpOnOutOfMemoryError and -XX:OnOutOfMemoryError support
  // 2. Post a JVMTI_EVENT_RESOURCE_EXHAUSTED event
  // 3. Set a pending OutOfMemoryError exception
  // 4. Return NULL
  // Compiled code must ensure these stubs are not called twice for the
  // same allocation site as 1 and 2 are visible side effects of failed
  // allocation that must not be repeated.
  static void new_instance(JavaThread* thread, Klass* klass) { new_instance_common(thread, klass, false); }
  static void new_array(JavaThread* thread, Klass* klass, jint length) { new_array_common(thread, klass, length, false); }
  static void new_multi_array(JavaThread* thread, Klass* klass, int rank, jint* dims) { new_multi_array_common(thread, klass, rank, dims, false); }
  static void dynamic_new_array(JavaThread* thread, oopDesc* element_mirror, jint length) { dynamic_new_array_common(thread, element_mirror, length, false); }
  static void dynamic_new_instance(JavaThread* thread, oopDesc* type_mirror) { dynamic_new_instance_common(thread, type_mirror, false); }

  // When allocation fails, these stubs return NULL.
  // Compiled code can use these stubs to retry a failed allocation.
  static void new_instance_or_null(JavaThread* thread, Klass* klass) { new_instance_common(thread, klass, true); }
  static void new_array_or_null(JavaThread* thread, Klass* klass, jint length) { new_array_common(thread, klass, length, true); }
  static void new_multi_array_or_null(JavaThread* thread, Klass* klass, int rank, jint* dims) { new_multi_array_common(thread, klass, rank, dims, true); }
  static void dynamic_new_array_or_null(JavaThread* thread, oopDesc* element_mirror, jint length) { dynamic_new_array_common(thread, element_mirror, length, true); }
  static void dynamic_new_instance_or_null(JavaThread* thread, oopDesc* type_mirror) { dynamic_new_instance_common(thread, type_mirror, true); }

  static jboolean thread_is_interrupted(JavaThread* thread, oopDesc* obj, jboolean clear_interrupted);
  static void vm_message(jboolean vmError, jlong format, jlong v1, jlong v2, jlong v3);
  static jint identity_hash_code(JavaThread* thread, oopDesc* obj);
  static address exception_handler_for_pc(JavaThread* thread);
  static void monitorenter(JavaThread* thread, oopDesc* obj, BasicLock* lock);
  static void monitorexit (JavaThread* thread, oopDesc* obj, BasicLock* lock);
  static void vm_error(JavaThread* thread, jlong where, jlong format, jlong value);
  static oopDesc* load_and_clear_exception(JavaThread* thread);
  static void log_printf(JavaThread* thread, const char* format, jlong v1, jlong v2, jlong v3);
  static void log_primitive(JavaThread* thread, jchar typeChar, jlong value, jboolean newline);
  // Print the passed in object, optionally followed by a newline.  If
  // as_string is true and the object is a java.lang.String then it
  // printed as a string, otherwise the type of the object is printed
  // followed by its address.
  static void log_object(JavaThread* thread, oopDesc* object, bool as_string, bool newline);
  static void write_barrier_pre(JavaThread* thread, oopDesc* obj);
  static void write_barrier_post(JavaThread* thread, void* card);
  static jboolean validate_object(JavaThread* thread, oopDesc* parent, oopDesc* child);
  static void new_store_pre_barrier(JavaThread* thread);

  // used to throw exceptions from compiled JVMCI code
  static void throw_and_post_jvmti_exception(JavaThread* thread, const char* exception, const char* message);
  // helper methods to throw exception with complex messages
  static void throw_klass_external_name_exception(JavaThread* thread, const char* exception, Klass* klass);
  static void throw_class_cast_exception(JavaThread* thread, const char* exception, Klass* caster_klass, Klass* target_klass);

  // Test only function
  static int test_deoptimize_call_int(JavaThread* thread, int value);
};

// Tracing macros.

#define IF_TRACE_jvmci_1 if (!(JVMCITraceLevel >= 1)) ; else
#define IF_TRACE_jvmci_2 if (!(JVMCITraceLevel >= 2)) ; else
#define IF_TRACE_jvmci_3 if (!(JVMCITraceLevel >= 3)) ; else
#define IF_TRACE_jvmci_4 if (!(JVMCITraceLevel >= 4)) ; else
#define IF_TRACE_jvmci_5 if (!(JVMCITraceLevel >= 5)) ; else

#define TRACE_jvmci_1 if (!(JVMCITraceLevel >= 1 && (tty->print("JVMCITrace-1: "), true))) ; else tty->print_cr
#define TRACE_jvmci_2 if (!(JVMCITraceLevel >= 2 && (tty->print("   JVMCITrace-2: "), true))) ; else tty->print_cr
#define TRACE_jvmci_3 if (!(JVMCITraceLevel >= 3 && (tty->print("      JVMCITrace-3: "), true))) ; else tty->print_cr
#define TRACE_jvmci_4 if (!(JVMCITraceLevel >= 4 && (tty->print("         JVMCITrace-4: "), true))) ; else tty->print_cr
#define TRACE_jvmci_5 if (!(JVMCITraceLevel >= 5 && (tty->print("            JVMCITrace-5: "), true))) ; else tty->print_cr

#endif // SHARE_VM_JVMCI_JVMCI_RUNTIME_HPP
