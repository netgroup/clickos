/*
 * alignmentinfo.{cc,hh} -- element stores alignment information
 * Eddie Kohler
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology.
 *
 * This software is being provided by the copyright holders under the GNU
 * General Public License, either version 2 or, at your discretion, any later
 * version. For more information, see the `COPYRIGHT' file in the source
 * distribution.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "alignmentinfo.hh"
#include "glue.hh"
#include "confparse.hh"
#include "router.hh"
#include "error.hh"

AlignmentInfo::AlignmentInfo()
{
}

bool
AlignmentInfo::configure_first() const
{
  return true;
}

int
AlignmentInfo::configure(const String &conf, ErrorHandler *errh)
{
  // check for an earlier AlignmentInfo
  int my_number = router()->eindex(this);
  const Vector<Element *> &ev = router()->elements();
  for (int i = 0; i < my_number; i++)
    if (AlignmentInfo *ai = (AlignmentInfo *)ev[i]->is_a_cast("AlignmentInfo"))
      return ai->configure(conf, errh);

  // this is the first AlignmentInfo; store all information here
  Vector<String> args;
  cp_argvec(conf, args);
  for (int i = 0; i < args.size(); i++) {
    Vector<String> parts;
    cp_spacevec(args[i], parts);
    
    if (parts.size() == 0)
      errh->warning("empty configuration argument %d", i);
    
    else if (Element *e = router()->find(this, parts[0], 0)) {
      int number = router()->eindex(e);
      if (_elem_offset.size() <= number) {
	_elem_offset.resize(number + 1, -1);
	_elem_icount.resize(number + 1, -1);
      }
      // report an error if different AlignmentInfo is given
      int old_offset = _elem_offset[number];
      int old_icount = _elem_icount[number];
      if (parts.size() % 2 != 1)
	errh->error("expected `ELEMENTNAME CHUNK OFFSET [CHUNK OFFSET...]'");
      _elem_offset[number] = _chunks.size();
      _elem_icount[number] = (parts.size() - 1) / 2;
      for (int j = 1; j < parts.size() - 1; j += 2) {
	int c = -1, o = -1;
	if (!cp_integer(parts[j], c))
	  errh->error("expected CHUNK");
	if (!cp_integer(parts[j+1], o))
	  errh->error("expected OFFSET");
	_chunks.push_back(c);
	_offsets.push_back(o);
      }
      // check for conflicting information on duplicate AlignmentInfo
      if (old_offset >= 0
	  && (old_icount != _elem_icount[number]
	      || memcmp(&_chunks[old_offset], &_chunks[_elem_offset[number]],
			old_icount * sizeof(int)) != 0
	      || memcmp(&_offsets[old_offset], &_offsets[_elem_offset[number]],
			old_icount * sizeof(int)) != 0))
	errh->error("conflicting AlignmentInfo for `%s'", parts[0].cc());
      
    }
  }
  
  return 0;
}

bool
AlignmentInfo::query1(Element *e, int port, int &chunk, int &offset) const
{
  int idx = router()->eindex(e);
  if (idx < 0 || idx >= _elem_offset.size() || _elem_offset[idx] < 0
      || port >= _elem_icount[idx])
    return false;
  else {
    chunk = _chunks[ _elem_offset[idx] + port ];
    offset = _offsets[ _elem_offset[idx] + port ];
    return true;
  }
}

bool
AlignmentInfo::query(Element *e, int port, int &chunk, int &offset)
{
  const Vector<Element *> &ev = e->router()->elements();
  for (int i = 0; i < ev.size(); i++)
    if (AlignmentInfo *ai = (AlignmentInfo *)ev[i]->is_a_cast("AlignmentInfo"))
      return ai->query1(e, port, chunk, offset);
  return false;
}

EXPORT_ELEMENT(AlignmentInfo)
