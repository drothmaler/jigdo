/* $Id: jigdo-io.hh,v 1.3 2003-09-03 19:28:13 atterer Exp $ -*- C++ -*-
  __   _
  |_) /|  Copyright (C) 2003  |  richard@
  | \/�|  Richard Atterer     |  atterer.net
  � '` �
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2. See
  the file COPYING for details.

  IO object for .jigdo downloads; download, gunzip, interpret

  Data (=downloaded bytes, status info) flows as follows:
  class:       Download -> SingleUrl -> JigdoIO -> GtkSingleUrl
  data member:              source        this        frontend

  The JigdoIO owns the SingleUrl (and the Download *object* inside it), but
  it doesn't own the GtkSingleUrl.

*/

#ifndef JIGDO_IO_HH
#define JIGDO_IO_HH

#include <datasource.hh>
#include <gunzip.hh>
#include <jigdo-io.fh>
#include <makeimagedl.hh>
#include <md5sum.fh>
#include <nocopy.hh>
//______________________________________________________________________

namespace Job {
  class JigdoIO;
}

class Job::JigdoIO : NoCopy, public Job::DataSource::IO, Gunzip::IO {
public:
  /** Create a new JigdoIO which is owned by m, gets data from download (will
      register itself with download's IOPtr) and passes it on to childIo.
      @param c Object which owns us (it is the MakeImageDl's child, but our
      master)
      @param download Gives the data of the .jigdo file to us
      @param childIo Provided by the frontend, e.g. a GtkSingleUrl object */
  JigdoIO(MakeImageDl::Child* c, DataSource::IO* frontendIo);
  ~JigdoIO();
  virtual Job::IO* job_removeIo(Job::IO* rmIo);

  inline MakeImageDl* master() const;
  inline DataSource* source() const;

private:
  /* Create object for an [Include]d file */
//   JigdoIO(MakeImageDl::Child* c, DataSource::IO* frontendIo,
//           JigdoIO* parent, unsigned inclLine);

  /** @return Root object of the include tree */
  inline JigdoIO* root();
  inline const JigdoIO* root() const;
  /** @return true iff this object is the root of the include tree. */
  inline bool isRoot() const;
  /** Return the ptr to the image section candidate object; the JigdoIO which
      might or might not contain the first [Image] section. If new data is
      received for that object and that new data contains an [Image], we know
      it's the first [Image]. If all data is recvd without any [Image]
      turning up, we continue walking the include tree depth-first. */
  inline JigdoIO* imgSectCandidate() const;
  /** Set the ptr to the image section candidate object */
  inline void setImgSectCandidate(JigdoIO* c);

  // Create error message with URL and line number
  void generateError(const char* msg);
  // True after above was called
  inline bool failed() const;
  // Called by gunzip_data(): New .jigdo line ready. Arg is empty on exit.
  void jigdoLine(string* l);

  // Virtual methods from DataSource::IO
  virtual void job_deleted();
  virtual void job_succeeded();
  virtual void job_failed(string* message);
  virtual void job_message(string* message);
  virtual void dataSource_dataSize(uint64 n);
  virtual void dataSource_data(const byte* data, size_t size,
                               uint64 currentSize);

  // Virtual methods from Gunzip::IO
  virtual void gunzip_deleted();
  virtual void gunzip_data(Gunzip*, byte* decompressed, unsigned size);
  virtual void gunzip_needOut(Gunzip*);
  virtual void gunzip_failed(string* message);

  MakeImageDl::Child* childDl;
  DataSource::IO* frontend; // Object provided by frontend for this download

  /* Representation of the tree of [Include] directives. Most of the time,
     the order of data in the .jigdo files is not relevant, with one
     exception: We must interpret the first [Image] section only, and ignore
     all following ones. */
  JigdoIO* parent; // .jigdo file which [Include]d us, or null if top-level
  unsigned includeLine; // If parent!=null, line num of [Include] in parent
  JigdoIO* firstChild; // First file we [Include], or null if none
  JigdoIO* next; // Right sibling, or null if none
  /* For the root object, contains imgSectCandidate, else ptr to root object.
     Don't access directly, use accessor methods. */
  JigdoIO* rootAndImageSectionCandidate;

  /* Transparent gunzipping of .jigdo file. GUNZIP_BUF_SIZE is also the max
     size a single line in the .jigdo is allowed to have */
  static const unsigned GUNZIP_BUF_SIZE = 16384;
  byte gunzipBuf[GUNZIP_BUF_SIZE];
  Gunzip gunzip;

  unsigned line; // Line number, for error messages
  string section; // Section name, empty if none yet, single null byte if err

  // Info about first image section of this .jigdo, if any
  unsigned imageSectionLine; // 0 if no [Image] found yet
  string imageName;
  string imageInfo, imageShortInfo;
  MD5* templateMd5;
};
//______________________________________________________________________

Job::JigdoIO* Job::JigdoIO::root() {
  if (isRoot()) return this; else return rootAndImageSectionCandidate;
}
const Job::JigdoIO* Job::JigdoIO::root() const {
  if (isRoot()) return this; else return rootAndImageSectionCandidate;
}
bool Job::JigdoIO::isRoot() const {
  return parent == 0;
}

Job::JigdoIO* Job::JigdoIO::imgSectCandidate() const {
  if (isRoot())
    return rootAndImageSectionCandidate;
  else
    return rootAndImageSectionCandidate->rootAndImageSectionCandidate;
}
void Job::JigdoIO::setImgSectCandidate(JigdoIO* c) {
  if (isRoot())
    rootAndImageSectionCandidate = c;
  else
    rootAndImageSectionCandidate->rootAndImageSectionCandidate = c;
}

Job::MakeImageDl* Job::JigdoIO::master() const { return childDl->master(); }
Job::DataSource*  Job::JigdoIO::source() const { return childDl->source(); }

bool Job::JigdoIO::failed() const {
  return (section.size() == 1 && section[0] == '\0');
}

#endif