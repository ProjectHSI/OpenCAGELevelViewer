// pch.cpp: source file corresponding to the pre-compiled header

#include "pch.h"

msclr::gcroot < msclr::interop::marshal_context ^ > msclr_context = gcnew msclr::interop::marshal_context();

// When you are using pre-compiled headers, this source file is necessary for compilation to succeed.
