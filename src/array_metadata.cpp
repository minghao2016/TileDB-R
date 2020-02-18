
// questions:
//  - simple accessor functions or s4 instances or XPtr?
//      -> s4 instance could cache more easily
//      -> XPtr easiest
//  - base layer here and refined interface in R function?
//      -> add s4 instance on R side
//      -> XPtr easier
//  - other functionality:
//      consolidate_metadata ?
//      [DONE] delete_metadata

#include <tiledb.h>

#define STRICT_R_HEADERS
#include <Rcpp.h>

using namespace tiledb;

// forward declaration
const char* _tiledb_datatype_to_string(tiledb_datatype_t dtype);

// ---- has_metadata
bool has_metadata_impl(tiledb::Array& array, const std::string key) {
  tiledb_datatype_t v_type;
  return array.has_metadata(key.c_str(), &v_type);
}

// [[Rcpp::export]]
bool has_metadata_simple(const std::string array_name, const std::string key) {
  Context ctx;                                  // Create TileDB context
  Array array(ctx, array_name, TILEDB_READ);    // Open array for reading
  return has_metadata_impl(array, key);
}

// [[Rcpp::export]]
bool has_metadata_ptr(Rcpp::XPtr<tiledb::Array> array, const std::string key) {
  return has_metadata_impl(*array, key);
}


// ---- metadata_num
int num_metadata_impl(tiledb::Array& array) {
  uint64_t nm = array.metadata_num();
  return static_cast<int>(nm);
}

// [[Rcpp::export]]
int num_metadata_simple(const std::string array_name) {
  Context ctx;                                  // Create TileDB context
  Array array(ctx, array_name, TILEDB_READ);    // Open array for reading
  return num_metadata_impl(array);
}

// [[Rcpp::export]]
int num_metadata_ptr(Rcpp::XPtr<tiledb::Array> array) {
  return num_metadata_impl(*array);
}

template <typename T>
Rcpp::IntegerVector copy_int_vector(const uint32_t v_num, const void* v) {
  // Strictly speaking a check for under/overflow would be needed here yet this for
  // metadata annotation (and not data payload) so extreme ranges are less likely
  Rcpp::IntegerVector vec(v_num);
  const T *ivec = static_cast<const T*>(v);
  size_t n = static_cast<size_t>(v_num);
  for (size_t i=0; i<n; i++) vec[i] = static_cast<int32_t>(ivec[i]);
  return(vec);
}

// ---- get_metadata
SEXP convert_vector_to_sexp(const tiledb_datatype_t v_type, const uint32_t v_num, const void* v) {
  // This supports a limited set of basic types as the metadata
  // annotation is not meant to support complete serialization
  if (v_type == TILEDB_INT32) {
    Rcpp::IntegerVector vec(v_num);
    std::memcpy(vec.begin(), v, v_num*sizeof(int32_t));
    return(vec);
  } else if (v_type == TILEDB_FLOAT64) {
    Rcpp::NumericVector vec(v_num);
    std::memcpy(vec.begin(), v, v_num*sizeof(double));
    return(vec);
  } else if (v_type == TILEDB_FLOAT32) {
    Rcpp::NumericVector vec(v_num);
    const float *fvec = static_cast<const float*>(v);
    size_t n = static_cast<size_t>(v_num);
    for (size_t i=0; i<n; i++) vec[i] = static_cast<double>(fvec[i]);
    return(vec);
  } else if (v_type == TILEDB_CHAR || v_type == TILEDB_STRING_ASCII) {
    Rcpp::CharacterVector vec(1);
    std::string s(static_cast<const char*>(v));
    s.resize(v_num);        // incoming char* not null terminated, ensures v_num bytes and terminate
    return(Rcpp::wrap(s));
  } else if (v_type == TILEDB_INT8) {
    Rcpp::LogicalVector vec(v_num);
    const int8_t *ivec = static_cast<const int8_t*>(v);
    size_t n = static_cast<size_t>(v_num);
    for (size_t i=0; i<n; i++) vec[i] = static_cast<bool>(ivec[i]);
    return(vec);
  } else if (v_type == TILEDB_UINT8) {
    // Strictly speaking a check for under/overflow would be needed here (and below) yet this
    // is for metadata annotation (and not data payload) so extreme ranges are less likely
    return copy_int_vector<uint8_t>(v_num, v);
  } else if (v_type == TILEDB_INT16) {
    return copy_int_vector<int16_t>(v_num, v);
  } else if (v_type == TILEDB_UINT16) {
    return copy_int_vector<uint16_t>(v_num, v);
  } else if (v_type == TILEDB_UINT32) {
    return copy_int_vector<uint32_t>(v_num, v);
  } else if (v_type == TILEDB_INT64) {
    return copy_int_vector<int64_t>(v_num, v);
  } else if (v_type == TILEDB_UINT64) {
    return copy_int_vector<uint64_t>(v_num, v);
  } else {
    Rcpp::stop("No support yet for %s", _tiledb_datatype_to_string(v_type));
  }
}

SEXP get_metadata_impl(tiledb::Array& array, const std::string key) {
 // Read with key
  tiledb_datatype_t v_type;
  uint32_t v_num;
  const void* v;
  array.get_metadata(key.c_str(), &v_type, &v_num, &v);
  if (v == NULL) {
    return R_NilValue;
  }

  return convert_vector_to_sexp(v_type, v_num, v);
}

// [[Rcpp::export]]
SEXP get_metadata_simple(const std::string array_name, const std::string key) {
  Context ctx;                                  // Create TileDB context
  Array array(ctx, array_name, TILEDB_READ);    // Open array for reading
  return get_metadata_impl(array, key);
}

// [[Rcpp::export]]
SEXP get_metadata_ptr(Rcpp::XPtr<tiledb::Array> array, const std::string key) {
  return get_metadata_impl(*array, key);
}


// ---- put_metadata
bool put_metadata_impl(tiledb::Array array, const std::string key, const SEXP obj) {
  // May want to add a mapper from SEXP type to tiledb type in libtiledb.cpp
  switch(TYPEOF(obj)) {
    case VECSXP: {
      Rcpp::stop("List objects are not supported.");
      array.close();
      return false;
      break;// not reached
    }
    case REALSXP: {
      Rcpp::NumericVector v(obj);
      array.put_metadata(key.c_str(), TILEDB_FLOAT64, v.size(), v.begin());
     break;
    }
    case INTSXP: {
      Rcpp::IntegerVector v(obj);
      array.put_metadata(key.c_str(), TILEDB_INT32, v.size(), v.begin());
      break;
    }
    case STRSXP: {
      Rcpp::CharacterVector v(obj);
      std::string s(v[0]);
      // We use TILEDB_CHAR interchangeably with TILEDB_STRING_ASCII is this best string type?
      array.put_metadata(key.c_str(), TILEDB_CHAR, s.length(), s.c_str());
      break;
    }
    case LGLSXP: {              // experimental: map R logical (ie TRUE, FALSE, NA) to int8
      Rcpp::LogicalVector v(obj);
      size_t n = static_cast<size_t>(v.size());
      std::vector<int8_t> ints(n);
      for (size_t i=0; i<n; i++) ints[i] = static_cast<int8_t>(v[i]);
      array.put_metadata(key.c_str(), TILEDB_INT8, ints.size(), ints.data());
      break;
    }
    default: {
      Rcpp::stop("No support (yet) for type '%d'.", TYPEOF(obj));
      array.close();
      return false;
      break; // not reached
    }
  }
  // Close array - Important so that the metadata get flushed
  array.close();
  return true;
}

// [[Rcpp::export]]
bool put_metadata_simple(const std::string array_name, const std::string key, const SEXP obj) {
  Context ctx;                                  // Create TileDB context
  Array array(ctx, array_name, TILEDB_WRITE);   // Open array for writing
  return put_metadata_impl(array, key, obj);
}

// [[Rcpp::export]]
bool put_metadata_ptr(Rcpp::XPtr<tiledb::Array> array, const std::string key, const SEXP obj) {
  return put_metadata_impl(*array, key, obj);
}


// ---- get by index
SEXP get_metadata_from_index_impl(tiledb::Array& array, const int idx) {
  std::string key;
  tiledb_datatype_t v_type;
  uint32_t v_num;
  const void* v;
  array.get_metadata_from_index(static_cast<uint64_t>(idx), &key, &v_type, &v_num, &v);
  if (v == NULL) {
    return R_NilValue;
  }

  RObject vec = convert_vector_to_sexp(v_type, v_num, v);
  vec.attr("names") = Rcpp::CharacterVector::create(key);
  return vec;
}

// [[Rcpp::export]]
SEXP get_metadata_from_index_ptr(Rcpp::XPtr<tiledb::Array> array, const int idx) {
  return get_metadata_from_index_impl(*array, idx);
}

// [[Rcpp::export]]
SEXP get_metadata_from_index_simple(const std::string array_name, const int idx) {
  Context ctx;                                  // Create TileDB context
  Array array(ctx, array_name, TILEDB_READ);    // Open array for reading
  return get_metadata_from_index_impl(array, idx);
}

// ---- get all metadata
SEXP get_all_metadata_impl(tiledb::Array& array) {
  uint64_t nm = array.metadata_num();
  int n = static_cast<int>(nm);
  Rcpp::List lst(n);
  Rcpp::CharacterVector names(n);
  for (auto i=0; i<n; i++) {
    // we trick this a little by having the returned object also carry an attribute
    // cleaner way (in a C++ pure sense) would be to return a pair of string and SEXP
    SEXP v = get_metadata_from_index_impl(array, i);
    Rcpp::RObject obj(v);
    Rcpp::CharacterVector objnms = obj.attr("names");
    names(i) = objnms[0];
    obj.attr("names") = R_NilValue; // remove attribute from object
    lst(i) = obj;
  }
  lst.attr("names") = names;
  return lst;

}

// [[Rcpp::export]]
SEXP get_all_metadata_simple(const std::string array_name) {
  Context ctx;                                  // Create TileDB context
  Array array(ctx, array_name, TILEDB_READ);    // Open array for reading
  return get_all_metadata_impl(array);
}

// [[Rcpp::export]]
SEXP get_all_metadata_ptr(Rcpp::XPtr<tiledb::Array> array) {
  return get_all_metadata_impl(*array);
}


// ---- delete metadata
void delete_metadata_impl(tiledb::Array& array, const std::string key) {
  array.delete_metadata(key);
}

// [[Rcpp::export]]
bool delete_metadata_simple(const std::string array_name, const std::string key) {
  Context ctx;                                  // Create TileDB context
  Array array(ctx, array_name, TILEDB_WRITE);   // Open array for reading
  delete_metadata_impl(array, key);
  return true;
}

// [[Rcpp::export]]
bool delete_metadata_ptr(Rcpp::XPtr<tiledb::Array> array, const std::string key) {
  delete_metadata_impl(*array, key);
  return true;
}