/*
 * Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
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
package jdk.vm.ci.hotspot;

import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

import jdk.vm.ci.code.CompilationRequest;
import jdk.vm.ci.code.CompilationRequestResult;
import jdk.vm.ci.common.JVMCIError;
import jdk.vm.ci.common.NativeImageReinitialize;
import jdk.vm.ci.hotspot.HotSpotJVMCIRuntime.Option;
import jdk.vm.ci.runtime.JVMCICompiler;
import jdk.vm.ci.runtime.JVMCICompilerFactory;
import jdk.vm.ci.runtime.JVMCIRuntime;
import jdk.vm.ci.services.JVMCIPermission;
import jdk.vm.ci.services.JVMCIServiceLocator;
import jdk.vm.ci.services.Services;

final class HotSpotJVMCICompilerConfig {

    /**
     * This factory allows JVMCI initialization to succeed but raises an error if the VM asks JVMCI
     * to perform a compilation. This allows the reflective parts of the JVMCI API to be used
     * without requiring a compiler implementation to be available.
     */
    private static class DummyCompilerFactory implements JVMCICompilerFactory, JVMCICompiler {

        private final String reason;

        DummyCompilerFactory(String reason) {
            this.reason = reason;
        }

        @Override
        public CompilationRequestResult compileMethod(CompilationRequest request) {
            throw new JVMCIError("No JVMCI compiler selected. " + reason);
        }

        @Override
        public String getCompilerName() {
            return "null";
        }

        @Override
        public JVMCICompiler createCompiler(JVMCIRuntime runtime) {
            return this;
        }
    }

    /**
     * Factory of the selected system compiler.
     */
    @NativeImageReinitialize private static JVMCICompilerFactory compilerFactory;

    /**
     * Gets the selected system compiler factory.
     *
     * @return the selected system compiler factory
     * @throws SecurityException if a security manager is present and it denies
     *             {@link JVMCIPermission} for any {@link JVMCIServiceLocator} loaded by this method
     */
    static JVMCICompilerFactory getCompilerFactory() {
        if (compilerFactory == null) {
            JVMCICompilerFactory factory = null;
            String compilerName = Option.Compiler.getString();
            if (compilerName != null) {
                if (compilerName.isEmpty() || compilerName.equals("null")) {
                    factory = new DummyCompilerFactory("Value of " + Option.Compiler.getPropertyName() + " property is \"" +
                                    compilerName + "\" which denotes the null JVMCI compiler.");
                } else {
                    for (JVMCICompilerFactory f : getJVMCICompilerFactories()) {
                        if (f.getCompilerName().equals(compilerName)) {
                            factory = f;
                        }
                    }
                    if (factory == null) {
                        throw new JVMCIError("JVMCI compiler \"%s\" not found", compilerName);
                    }
                }
            } else {
                // Auto select a single available compiler
                List<String> multiple = null;
                for (JVMCICompilerFactory f : getJVMCICompilerFactories()) {
                    if (multiple != null) {
                        multiple.add(f.getCompilerName());
                    } else if (factory == null) {
                        factory = f;
                    } else {
                        multiple = new ArrayList<>();
                        multiple.add(f.getCompilerName());
                        multiple.add(factory.getCompilerName());
                        factory = null;
                    }
                }
                if (multiple != null) {
                    factory = new DummyCompilerFactory("Multiple providers of " + JVMCICompilerFactory.class + " available: " +
                                    String.join(", ", multiple) +
                                    ". You can select one of these with the " + Option.Compiler.getPropertyName() + " property " +
                                    "(e.g., -D" + Option.Compiler.getPropertyName() + "=" + multiple.get(0) + ").");
                } else if (factory == null) {
                    Path jvmciDir = Paths.get(Services.getSavedProperties().get("java.home"), "lib", "jvmci");
                    factory = new DummyCompilerFactory("No providers of " + JVMCICompilerFactory.class + " found in " + jvmciDir +
                                    " or on the class path specified by the jvmci.class.path.append property.");
                }
            }
            factory.onSelection();
            compilerFactory = factory;
        }
        return compilerFactory;
    }

    private static List<JVMCICompilerFactory> getJVMCICompilerFactories() {
        return JVMCIServiceLocator.getProviders(JVMCICompilerFactory.class);
    }
}
