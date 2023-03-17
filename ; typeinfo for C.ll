; typeinfo for C
@_ZTI1C = linkonce_odr dso_local constant { ptr, ptr, ptr } { ptr getelementptr inbounds (ptr, ptr @_ZTVN10__cxxabiv120__si_class_type_infoE, i64 2), ptr @_ZTS1C, ptr @_ZTI1A }, comdat, align 8

; vtable for C
@_ZTV1C = linkonce_odr dso_local unnamed_addr constant { [3 x ptr] } { [3 x ptr] [ptr null, ptr @_ZTI1C, ptr @_ZN1C1aEv] }, comdat, align 8

; vtable for C
@_ZTV1C = linkonce_odr dso_local unnamed_addr constant { [4 x ptr], [3 x ptr] } { [4 x ptr] [ptr null, ptr @_ZTI1C, ptr @_ZN1C1aEv, ptr @_ZN1C1bEv], [3 x ptr] [ptr inttoptr (i64 -8 to ptr), ptr @_ZTI1C, ptr @_ZThn8_N1C1bEv] }, comdat, align 8



; int square()
define dso_local noundef i32 @_Z6squarei(i32 noundef %0) #0 !dbg !10 {
  %1 = alloca ptr, align 8                                                                    ; decl A *o
  %2 = call noalias noundef nonnull ptr @_Znwm(i64 noundef 8) #4, !dbg !14                    ; call new(8)
  call void @_ZN1CC2Ev(ptr noundef nonnull align 8 dereferenceable(8) %2) #5, !dbg !15        ; call C::C() on that ptr
  store ptr %2, ptr %1, align 8, !dbg !16                                                     ; store ptr in o
  %3 = load ptr, ptr %1, align 8, !dbg !17                                                    ; read ptr to object from *o
  %4 = load ptr, ptr %3, align 8, !dbg !18                                                    ; read ptr to vtable from o 
  %5 = getelementptr inbounds ptr, ptr %4, i64 0, !dbg !18                                    ; get the address of the 0th element of the ptr array (virtual method 0?)
  %6 = load ptr, ptr %5, align 8, !dbg !18                                                    ; read value at that address, which is a ptr to a function
  %7 = call noundef i32 %6(ptr noundef nonnull align 8 dereferenceable(8) %3), !dbg !18       ; call the function pointer
  ret i32 %7, !dbg !19                                                                        ; return
}

; C::C()
define linkonce_odr dso_local void @_ZN1CC2Ev(ptr noundef nonnull align 8 dereferenceable(8) %0) unnamed_addr #2 comdat align 2 !dbg !20 {
  %2 = alloca ptr, align 8                                                                    ; decl this
  store ptr %0, ptr %2, align 8                                                               ; store ptr passed as parameter to this
  %3 = load ptr, ptr %2, align 8                                                              ; load ptr to this
  call void @_ZN1AC2Ev(ptr noundef nonnull align 8 dereferenceable(8) %3) #5, !dbg !21        ; call A::A() with parameter of this
  store ptr getelementptr inbounds ({ [3 x ptr] }, ptr @_ZTV1C, i32 0, inrange i32 0, i32 2), ptr %3, align 8, !dbg !21 ; GEP = comp from C vtable the 3rd index. This is ptr to address of first virtual method.
                                                                                                                        ; store that value at address of this

  ret void, !dbg !21
}

; C::a()
define linkonce_odr dso_local noundef i32 @_ZN1C1aEv(ptr noundef nonnull align 8 dereferenceable(8) %0) unnamed_addr #3 comdat align 2 !dbg !24 {
  %2 = alloca ptr, align 8
  store ptr %0, ptr %2, align 8
  %3 = load ptr, ptr %2, align 8
  ret i32 2, !dbg !25
}

; A::A()
define linkonce_odr dso_local void @_ZN1AC2Ev(ptr noundef nonnull align 8 dereferenceable(8) %0) unnamed_addr #2 comdat align 2 !dbg !22 {
  %2 = alloca ptr, align 8              ; decl this
  store ptr %0, ptr %2, align 8         ; store ptr passed as parameter to this
  %3 = load ptr, ptr %2, align 8        ; load ptr to this
  store ptr getelementptr inbounds ({ [3 x ptr] }, ptr @_ZTV1A, i32 0, inrange i32 0, i32 2), ptr %3, align 8, !dbg !23 ; GEP = comp from A vtable the 3rd index. This is ptr to address of first virtual method.
                                                                                                                        ; store that value at address of this
  ret void, !dbg !23
}