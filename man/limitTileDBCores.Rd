% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/Config.R
\name{limitTileDBCores}
\alias{limitTileDBCores}
\title{Limit TileDB core use to a given number of cores}
\usage{
limitTileDBCores(ncores, verbose = FALSE)
}
\arguments{
\item{ncores}{Value of CPUs used, if missing the smaller of a fallback of two, the value of
\sQuote{Ncpus} (if set) and the value of environment variable \sQuote{"OMP_THREAD_LIMIT"} is
used.}

\item{verbose}{Optional logical toggle; if set, a short message is displayed informing the
user about the value set.}
}
\value{
The modified configuration object is returned invisibly. As a side-effect the
updated config is also used to set the global context object.
}
\description{
By default, TileDB will use all available cores on a given machine. In multi-user or
multi-process settings, one may want to reduce the number of core. This function will
take a given number, or default to smaller of the \sQuote{Ncpus} options value or the
\sQuote{"OMP_THREAD_LIMIT"} enviroment variable (or two as hard fallback).
}
