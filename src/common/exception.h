/*-------------------------------------------------------------------------
 *
 * exception.h
 * file description
 *
 * Copyright(c) 2015, CMU
 *
 * /n-store/src/common/exception.h
 *
 *-------------------------------------------------------------------------
 */

#pragma once

#include <iostream>

#include "common/types.h"

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <stdexcept>
#include <execinfo.h>
#include <errno.h>
#include <cxxabi.h>
#include <signal.h>

namespace nstore {

//===--------------------------------------------------------------------===//
// Exception Types
//===--------------------------------------------------------------------===//

enum ExceptionType {
	EXCEPTION_TYPE_INVALID = 0, 				  	// invalid type

	EXCEPTION_TYPE_OUT_OF_RANGE = 1, 				// value out of range error
	EXCEPTION_TYPE_CONVERSION = 2, 					// conversion/casting error
	EXCEPTION_TYPE_UNKNOWN_TYPE = 3,				// unknown type
	EXCEPTION_TYPE_DECIMAL = 4,						  // decimal related
	EXCEPTION_TYPE_MISMATCH_TYPE = 5,				// type mismatch
	EXCEPTION_TYPE_DIVIDE_BY_ZERO = 6,			// divide by 0
	EXCEPTION_TYPE_OBJECT_SIZE = 7,					// object size exceeded
	EXCEPTION_TYPE_INCOMPATIBLE_TYPE = 8,		// incompatible for operation
	EXCEPTION_TYPE_SERIALIZATION = 9,			  // serialization
	EXCEPTION_TYPE_TRANSACTION = 10,        // transaction management
	EXCEPTION_TYPE_NOT_IMPLEMENTED = 11     // method not implemented
};

class Exception {
public:

	Exception(std::string message) {
		// print stack trace
		//PrintStackTrace();

		std::string exception_message =
				"============================================================================\n"
				"\tMessage :: " +	message + "\n"
				"============================================================================\n";
		std::cerr << exception_message;

		// TODO: raise HELL for now
		raise(SIGSEGV);
		throw std::runtime_error(exception_message);
	}

	Exception(ExceptionType exception_type, std::string message){
		// print stack trace
		//PrintStackTrace();

		std::string exception_message =
				"============================================================================\n"
				"\tException Type :: " + ExpectionTypeToString(exception_type) + "\n\tMessage :: " +	message + "\n"
				"============================================================================\n";

		std::cerr << exception_message;

		// TODO: raise HELL for now
		raise(SIGSEGV);
		throw std::runtime_error(exception_message);
	}

	std::string ExpectionTypeToString(ExceptionType type){
		switch (type) {
		case EXCEPTION_TYPE_INVALID:
			return "Invalid";
		case EXCEPTION_TYPE_OUT_OF_RANGE:
			return "Out of Range";
		case EXCEPTION_TYPE_CONVERSION:
			return "Conversion";
		case EXCEPTION_TYPE_UNKNOWN_TYPE:
			return "Unknown Type";
		case EXCEPTION_TYPE_DECIMAL:
			return "Decimal";
		case EXCEPTION_TYPE_MISMATCH_TYPE:
			return "Mismatch Type";
		case EXCEPTION_TYPE_DIVIDE_BY_ZERO:
			return "Divede by Zero";
		case EXCEPTION_TYPE_OBJECT_SIZE:
			return "Object Size";
		case EXCEPTION_TYPE_INCOMPATIBLE_TYPE:
			return "Incompatible type";
		case EXCEPTION_TYPE_SERIALIZATION:
			return "Serialization";

		default:
			return "Unknown";
		}
	}

	// Based on :: http://panthema.net/2008/0901-stacktrace-demangled/
	void PrintStackTrace(FILE *out = stderr, unsigned int max_frames = 63){
		fprintf(out, "Stack Trace:\n");

		/// storage array for stack trace address data
		void* addrlist[max_frames+1];

		/// retrieve current stack addresses
		int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));

		if (addrlen == 0) {
			fprintf(out, "  <empty, possibly corrupt>\n");
			return;
		}

		/// resolve addresses into strings containing "filename(function+address)",
		/// this array must be free()-ed
		char** symbol_list = backtrace_symbols(addrlist, addrlen);

		/// allocate string which will be filled with the demangled function name
		size_t func_name_size = 1024;
		char* func_name = (char*) malloc(func_name_size);

		/// iterate over the returned symbol lines. skip the first, it is the
		/// address of this function.
		for (int i = 1; i < addrlen; i++){
			char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

			/// find parentheses and +address offset surrounding the mangled name:
			/// ./module(function+0x15c) [0x8048a6d]
			for (char *p = symbol_list[i]; *p; ++p){
				if (*p == '(')
					begin_name = p;
				else if (*p == '+')
					begin_offset = p;
				else if (*p == ')' && begin_offset) {
					end_offset = p;
					break;
				}
			}

			if (begin_name && begin_offset && end_offset && begin_name < begin_offset){
				*begin_name++ = '\0';
				*begin_offset++ = '\0';
				*end_offset = '\0';

				/// mangled name is now in [begin_name, begin_offset) and caller
				/// offset in [begin_offset, end_offset). now apply  __cxa_demangle():
				int status;
				char* ret = abi::__cxa_demangle(begin_name, func_name, &func_name_size, &status);
				if (status == 0) {
					func_name = ret; // use possibly realloc()-ed string
					fprintf(out, "  %s : %s+%s\n",
							symbol_list[i], func_name, begin_offset);
				}
				else {
					/// demangling failed. Output function name as a C function with
					/// no arguments.
					fprintf(out, "  %s : %s()+%s\n",
							symbol_list[i], begin_name, begin_offset);
				}
			}
			else
			{
				/// couldn't parse the line ? print the whole line.
				fprintf(out, "  %s\n", symbol_list[i]);
			}
		}

		free(func_name);
		free(symbol_list);
	}

};

//===--------------------------------------------------------------------===//
// Exception derived classes
//===--------------------------------------------------------------------===//

class CastException : Exception {
	CastException() = delete;

public:
	CastException(const ValueType origType, const ValueType newType) :
		Exception(EXCEPTION_TYPE_CONVERSION,
				"Type " + ValueToString(origType) + " can't be cast as " + ValueToString(newType)){
	}

};

class ValueOutOfRangeException : Exception {
	ValueOutOfRangeException() = delete;

public:
	ValueOutOfRangeException(const int64_t value, const ValueType origType, const ValueType newType) :
		Exception(EXCEPTION_TYPE_CONVERSION,
				"Type " +	ValueToString(origType) + " with value " + std::to_string((intmax_t)value) +
				" can't be cast as %s because the value is out of range for the destination type " +
				ValueToString(newType)){
	}

	ValueOutOfRangeException(const double value, const ValueType origType, const ValueType newType) :
		Exception(EXCEPTION_TYPE_CONVERSION,
				"Type " +	ValueToString(origType) + " with value " + std::to_string(value) +
				" can't be cast as %s because the value is out of range for the destination type " +
				ValueToString(newType)){
	}
};


class ConversionException : Exception {
	ConversionException() = delete;

public:
	ConversionException(std::string msg) :
		Exception(EXCEPTION_TYPE_CONVERSION, msg){
	}
};

class UnknownTypeException : Exception {
	UnknownTypeException() = delete;

public:
	UnknownTypeException(int type, std::string msg) :
		Exception(EXCEPTION_TYPE_UNKNOWN_TYPE, "unknown type " + std::to_string(type) + msg){
	}
};

class DecimalException : Exception {
	DecimalException() = delete;

public:
	DecimalException(std::string msg) :
		Exception(EXCEPTION_TYPE_DECIMAL, msg){
	}
};

class TypeMismatchException : Exception {
	TypeMismatchException() = delete;

public:
	TypeMismatchException(std::string msg, const ValueType type_1, const ValueType type_2) :
		Exception(EXCEPTION_TYPE_MISMATCH_TYPE,
				"Type " + ValueToString(type_1) + " does not match with " + ValueToString(type_2) + msg){
	}
};

class NumericValueOutOfRangeException : Exception {
	NumericValueOutOfRangeException() = delete;

public:
	NumericValueOutOfRangeException(std::string msg) :
		Exception(EXCEPTION_TYPE_OUT_OF_RANGE, msg){
	}
};

class DivideByZeroException : Exception {
	DivideByZeroException() = delete;

public:
	DivideByZeroException(std::string msg) :
		Exception(EXCEPTION_TYPE_DIVIDE_BY_ZERO, msg){
	}
};

class ObjectSizeException : Exception {
	ObjectSizeException() = delete;

public:
	ObjectSizeException(std::string msg) :
		Exception(EXCEPTION_TYPE_OBJECT_SIZE, msg){
	}
};

class IncompatibleTypeException : Exception {
	IncompatibleTypeException() = delete;

public:
	IncompatibleTypeException(int type, std::string msg) :
		Exception(EXCEPTION_TYPE_INCOMPATIBLE_TYPE, "Incompatible type " + std::to_string(type) + msg){
	}
};

class SerializationException : Exception {
	SerializationException() = delete;

public:
	SerializationException(std::string msg) :
		Exception(EXCEPTION_TYPE_SERIALIZATION, msg){
	}
};

class TransactionException : Exception {
  TransactionException() = delete;

public:
  TransactionException(std::string msg) :
    Exception(EXCEPTION_TYPE_TRANSACTION, msg){
  }
};

class NotImplementedException : Exception {
  NotImplementedException() = delete;

public:
  NotImplementedException(std::string msg) :
    Exception(EXCEPTION_TYPE_NOT_IMPLEMENTED, msg){
  }
};

} // End nstore namespace

