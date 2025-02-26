/*******************************************************************************
* Author    :  Angus Johnson                                                   *
* Date      :  27 October 2022                                                 *
* Website   :  http://www.angusj.com                                           *
* Copyright :  Angus Johnson 2010-2022                                         *
* Purpose   :  This module exports the Clipper2 Library (ie DLL/so)            *
* License   :  http://www.boost.org/LICENSE_1_0.txt                            *
*******************************************************************************/

// The exported functions below refer to simple structures that
// can be understood across multiple languages. Consequently
// Path64, PathD, Polytree64 etc are converted from classes
// (std::vector<> etc) into the following data structures:
//
// CPath64 (int64_t*) & CPathD (double_t*):
// Path64 and PathD are converted into arrays of x,y coordinates.
// However in these arrays the first x,y coordinate pair is a
// counter with 'x' containing the number of following coordinate
// pairs. ('y' must always be 0.)
//__________________________________
// |counter|coord1|coord2|...|coordN|
// |N ,0   |x1, y1|x2, y2|...|xN, yN|
// __________________________________
//
// CPaths64 (int64_t**) & CPathsD (double_t**):
// These are arrays of pointers to CPath64 and CPathD where
// the first pointer is to a 'counter 'path'. This 'counter
// path' has a single x,y coord pair where 'y' contains
// the number of paths that follow (and with 'x' always 0).
//_______________________________
// |counter|path1|path2|...|pathN|
// |addr0  |addr1|addr2|...|addrN|  (where *addr0 == 0,N)
// _______________________________
//
// The structures of CPolytree64 and CPolytreeD are defined
// below and don't need to be repeated or explained here.
// 
// The pointer structures created and exported through these
// functions can't safely be destroyed externally, so a number
// of 'dispose functions are also exported.

#ifndef CLIPPER2_EXPORT_H
#define CLIPPER2_EXPORT_H

#include <cstdlib>
#include <vector>

#include "clipper2/clipper.core.h"
#include "clipper2/clipper.engine.h"
#include "clipper2/clipper.offset.h"
#include "clipper2/clipper.rectclip.h"

#include <fstream> // debugging


namespace Clipper2Lib {

typedef int64_t* CPath64;
typedef int64_t** CPaths64;
typedef double* CPathD;
typedef double** CPathsD;

typedef struct CRect64 { int64_t val[4]; } CRect64;
typedef struct CRectD { double val[4]; } CRectD;

typedef struct CPolyPath64 {
  CPath64       polygon;
  uint32_t      is_hole;
  uint32_t      child_count;
  CPolyPath64*  childs;
} 
CPolyTree64;

typedef struct CPolyPathD {
  CPathD        polygon;
  uint32_t      is_hole;
  uint32_t      child_count;
  CPolyPathD*   childs;
} 
CPolyTreeD;

inline CPath64 CreateCPath64(size_t cnt1, size_t cnt2);
inline CPath64 CreateCPath64(const Path64& p);
inline CPaths64 CreateCPaths64(const Paths64& pp);
inline Path64 ConvertCPath64(const CPath64& p);
inline Paths64 ConvertCPaths64(const CPaths64& pp);

inline CPathD CreateCPathD(size_t cnt1, size_t cnt2);
inline CPathD CreateCPathD(const PathD& p);
inline CPathsD CreateCPathsD(const PathsD& pp);
inline PathD ConvertCPathD(const CPathD& p);
inline PathsD ConvertCPathsD(const CPathsD& pp);

// the following function avoid multiple conversions
inline Path64 ConvertCPathD(const CPathD& p, double scale);
inline Paths64 ConvertCPathsD(const CPathsD& pp, double scale);
inline CPathD CreateCPathD(const Path64& p, double scale);
inline CPathsD CreateCPathsD(const Paths64& pp, double scale);

inline CPolyTree64* CreateCPolyTree64(const PolyTree64& pt);
inline CPolyTreeD* CreateCPolyTreeD(const PolyTree64& pt, double scale);

#define EXTERN_DLL_EXPORT extern "C" __declspec(dllexport)

EXTERN_DLL_EXPORT const char* Version()
{
  return CLIPPER2_VERSION;
}

EXTERN_DLL_EXPORT void DisposeExportedCPath64(CPath64 p)
{
  if (p) delete[] p;
}

EXTERN_DLL_EXPORT void DisposeExportedCPaths64(CPaths64& pp)
{
  if (!pp) return;
  CPaths64 v = pp;
  CPath64 cnts = *v;
  const size_t cnt = static_cast<size_t>(cnts[1]);
  for (size_t i = 0; i <= cnt; ++i) //nb: cnt +1
    DisposeExportedCPath64(*v++);
  delete[] pp;
  pp = nullptr;
}

EXTERN_DLL_EXPORT void DisposeExportedCPathD(CPathD p)
{
  if (p) delete[] p;
}

EXTERN_DLL_EXPORT void DisposeExportedCPathsD(CPathsD& pp)
{
  if (!pp) return;
  CPathsD v = pp;
  CPathD cnts = *v;
  size_t cnt = static_cast<size_t>(cnts[1]);
  for (size_t i = 0; i <= cnt; ++i) //nb: cnt +1
    DisposeExportedCPathD(*v++);
  delete[] pp;
  pp = nullptr;
}

EXTERN_DLL_EXPORT int BooleanOp64(uint8_t cliptype, 
  uint8_t fillrule, const CPaths64 subjects,
  const CPaths64 subjects_open, const CPaths64 clips,
  CPaths64& solution, CPaths64& solution_open,
  bool preserve_collinear = true, bool reverse_solution = false)
{
  if (cliptype > static_cast<uint8_t>(ClipType::Xor))
    return -4;
  if (fillrule > static_cast<uint8_t>(FillRule::Negative))
    return -3;
  Paths64 sub, sub_open, clp;
  sub       = ConvertCPaths64(subjects);
  sub_open  = ConvertCPaths64(subjects_open);
  clp       = ConvertCPaths64(clips);

  Paths64 sol, sol_open;
  Clipper64 clipper;
  clipper.PreserveCollinear = preserve_collinear;
  clipper.ReverseSolution = reverse_solution;
  if (sub.size() > 0) clipper.AddSubject(sub);
  if (sub_open.size() > 0) clipper.AddOpenSubject(sub_open);
  if (clp.size() > 0) clipper.AddClip(clp);
  if (!clipper.Execute(ClipType(cliptype), FillRule(fillrule), sol, sol_open)) 
    return -1; // clipping bug - should never happen :)
  solution = CreateCPaths64(sol);
  solution_open = CreateCPaths64(sol_open);
  return 0; //success :))
}

EXTERN_DLL_EXPORT int BooleanOpPt64(uint8_t cliptype,
  uint8_t fillrule, const CPaths64 subjects,
  const CPaths64 subjects_open, const CPaths64 clips,
  CPolyTree64*& solution, CPaths64& solution_open,
  bool preserve_collinear = true, bool reverse_solution = false)
{
  if (cliptype > static_cast<uint8_t>(ClipType::Xor))
    return -4;
  if (fillrule > static_cast<uint8_t>(FillRule::Negative))
    return -3;
  Paths64 sub, sub_open, clp;
  sub = ConvertCPaths64(subjects);
  sub_open = ConvertCPaths64(subjects_open);
  clp = ConvertCPaths64(clips);

  PolyTree64 pt;
  Paths64 sol_open;
  Clipper64 clipper;
  clipper.PreserveCollinear = preserve_collinear;
  clipper.ReverseSolution = reverse_solution;
  if (sub.size() > 0) clipper.AddSubject(sub);
  if (sub_open.size() > 0) clipper.AddOpenSubject(sub_open);
  if (clp.size() > 0) clipper.AddClip(clp);
  if (!clipper.Execute(ClipType(cliptype), FillRule(fillrule), pt, sol_open))
    return -1; // clipping bug - should never happen :)

  solution = CreateCPolyTree64(pt);
  solution_open = CreateCPaths64(sol_open);
  return 0; //success :))
}

EXTERN_DLL_EXPORT int BooleanOpD(uint8_t cliptype,
  uint8_t fillrule, const CPathsD subjects,
  const CPathsD subjects_open, const CPathsD clips,
  CPathsD& solution, CPathsD& solution_open, int precision = 2,
  bool preserve_collinear = true, bool reverse_solution = false)
{
  if (precision < -8 || precision > 8) return -5;
  if (cliptype > static_cast<uint8_t>(ClipType::Xor))
    return -4;
  if (fillrule > static_cast<uint8_t>(FillRule::Negative))
    return -3;
  const double scale = std::pow(10, precision);

  Paths64 sub, sub_open, clp, sol, sol_open;
  sub       = ConvertCPathsD(subjects, scale);
  sub_open  = ConvertCPathsD(subjects_open, scale);
  clp       = ConvertCPathsD(clips, scale);

  Clipper64 clipper;
  clipper.PreserveCollinear = preserve_collinear;
  clipper.ReverseSolution = reverse_solution;
  if (sub.size() > 0)
    clipper.AddSubject(sub);
  if (sub_open.size() > 0)
    clipper.AddOpenSubject(sub_open);
  if (clp.size() > 0)
    clipper.AddClip(clp);
  if (!clipper.Execute(ClipType(cliptype),
    FillRule(fillrule), sol, sol_open)) return -1;

  if (sol.size() > 0)
    solution = CreateCPathsD(sol, 1 / scale);
  if (sol_open.size() > 0)
    solution_open = CreateCPathsD(sol_open, 1 / scale);
  return 0;
}

EXTERN_DLL_EXPORT int BooleanOpPtD(uint8_t cliptype,
  uint8_t fillrule, const CPathsD subjects,
  const CPathsD subjects_open, const CPathsD clips,
  CPolyTreeD*& solution, CPathsD& solution_open, int precision = 2,
  bool preserve_collinear = true, bool reverse_solution = false)
{
  if (precision < -8 || precision > 8) return -5;
  if (cliptype > static_cast<uint8_t>(ClipType::Xor))
    return -4;
  if (fillrule > static_cast<uint8_t>(FillRule::Negative))
    return -3;
  const double scale = std::pow(10, precision);

  PolyTree64 sol;
  Paths64 sub, sub_open, clp, sol_open;
  sub       = ConvertCPathsD(subjects, scale);
  sub_open  = ConvertCPathsD(subjects_open, scale);
  clp       = ConvertCPathsD(clips, scale);

  Clipper64 clipper;
  clipper.PreserveCollinear = preserve_collinear;
  clipper.ReverseSolution = reverse_solution;
  if (sub.size() > 0)
    clipper.AddSubject(sub);
  if (sub_open.size() > 0)
    clipper.AddOpenSubject(sub_open);
  if (clp.size() > 0)
    clipper.AddClip(clp);
  if (!clipper.Execute(ClipType(cliptype),
    FillRule(fillrule), sol, sol_open)) return -1;

  solution = CreateCPolyTreeD(sol, 1 / scale);
  if (sol_open.size() > 0)
    solution_open = CreateCPathsD(sol_open, 1 / scale);
  return 0;
}

EXTERN_DLL_EXPORT CPaths64 InflatePaths64(const CPaths64 paths, 
  double delta, uint8_t jt, uint8_t et, double miter_limit = 2.0,
  double arc_tolerance = 0.0, bool reverse_solution = false)
{
  Paths64 pp;
  pp = ConvertCPaths64(paths);

  ClipperOffset clip_offset( miter_limit, 
    arc_tolerance, reverse_solution);
  clip_offset.AddPaths(pp, JoinType(jt), EndType(et));
  Paths64 result = clip_offset.Execute(delta);
  return CreateCPaths64(result);
}

EXTERN_DLL_EXPORT CPathsD InflatePathsD(const CPathsD paths, 
  double delta, uint8_t jt, uint8_t et,
  double precision = 2, double miter_limit = 2.0,
  double arc_tolerance = 0.0, bool reverse_solution = false)
{
  if (precision < -8 || precision > 8 || !paths) return nullptr;
  const double scale = std::pow(10, precision);
  ClipperOffset clip_offset(miter_limit, arc_tolerance, reverse_solution);
  Paths64 pp = ConvertCPathsD(paths, scale);
  clip_offset.AddPaths(pp, JoinType(jt), EndType(et));
  Paths64 result = clip_offset.Execute(delta * scale);
  return CreateCPathsD(result, 1/scale);
}

EXTERN_DLL_EXPORT CPaths64 RectClip64(const Rect64 rect, 
  const CPaths64 paths)
{
  if (rect.IsEmpty() || !paths) return nullptr;
  class RectClip rc(rect);
  Paths64 pp = ConvertCPaths64(paths);
  Paths64 result;
  result.reserve(pp.size());
  for (const Path64& p : pp)
  {
    Rect64 pathRec = Bounds(p);
    if (!rect.Intersects(pathRec)) 
      continue;    
    else if (rect.Contains(pathRec))
      result.push_back(p);
    else
    {
      Path64 p2 = rc.Execute(p);
      if (!p2.empty()) result.push_back(std::move(p2));
    }
  }
  return CreateCPaths64(result);
}

EXTERN_DLL_EXPORT CPathsD RectClipD(const RectD rect, 
  const CPathsD paths, int precision = 2)
{
  if (rect.IsEmpty() || !paths) return nullptr;
  if (precision < -8 || precision > 8) return nullptr;
  const double scale = std::pow(10, precision);
  Rect64 r = ScaleRect<int64_t, double>(rect, scale);
  Paths64 pp = ConvertCPathsD(paths, scale);

  class RectClip rc(r);
  Paths64 result;
  result.reserve(pp.size());
  for (const Path64& p : pp)
  {
    Rect64 pathRec = Bounds(p);
    if (!r.Intersects(pathRec))
      continue;
    else if (r.Contains(pathRec))
      result.push_back(p);
    else
    {
      Path64 p2 = rc.Execute(p);
      if (!p2.empty()) result.push_back(std::move(p2));
    }
  }
  return CreateCPathsD(result, 1/scale);
}

EXTERN_DLL_EXPORT CPaths64 RectClipLines64(const Rect64 rect, 
  const CPaths64 paths)
{
  if (rect.IsEmpty() || !paths) return nullptr;
  class RectClipLines rcl (rect);
  Paths64 pp = ConvertCPaths64(paths);
  Paths64 result;
  result.reserve(pp.size());

  for (const Path64& p : pp)
  {
    Rect64 pathRec = Bounds(p);
    if (!rect.Intersects(pathRec))
      continue;
    else if (rect.Contains(pathRec))
      result.push_back(p);
    else
    {
      Paths64 pp2 = rcl.Execute(p);
      if (!pp2.empty())
        result.insert(result.end(), pp2.begin(), pp2.end());
    }
  }
  return CreateCPaths64(result);
}

EXTERN_DLL_EXPORT CPathsD RectClipLinesD(const RectD rect, 
  const CPathsD paths, int precision = 2)
{
  Paths64 result;
  if (rect.IsEmpty() || !paths) return nullptr;
  if (precision < -8 || precision > 8) return nullptr;
  const double scale = std::pow(10, precision);
  Rect64 r = ScaleRect<int64_t, double>(rect, scale);
  class RectClipLines rcl(r);
  Paths64 pp = ConvertCPathsD(paths, scale);

  result.reserve(pp.size());
  for (const Path64& p : pp)
  {
    Rect64 pathRec = Bounds(p);
    if (!r.Intersects(pathRec))
      continue;
    else if (r.Contains(pathRec))
      result.push_back(p);
    else
    {
      Paths64 pp2 = rcl.Execute(p);
      if (pp2.empty()) continue;
      result.insert(result.end(), pp2.begin(), pp2.end());
    }
  }
  return CreateCPathsD(result, 1/scale);
}

inline CPath64 CreateCPath64(size_t cnt1, size_t cnt2)
{
  // create a dummy counter path 
  CPath64 result = new int64_t[2 + cnt1 *2];
  result[0] = cnt1;
  result[1] = cnt2;
  return result;
}

inline CPath64 CreateCPath64(const Path64& p)
{
  size_t cnt = p.size();
  if (!cnt) return nullptr;
  CPath64 result = CreateCPath64(cnt, 0);
  CPath64 v = result;
  v += 2; // skip counters
  for (const Point64& pt : p)
  {
    *v++ = pt.x;
    *v++ = pt.y;
  }
  return result;
}

inline Path64 ConvertCPath64(const CPath64& p)
{
  Path64 result;
  if (p && *p)
  {
    CPath64 v = p;
    const size_t cnt = static_cast<size_t>(p[0]);
    v += 2; // skip counters
    result.reserve(cnt);
    for (size_t i = 0; i < cnt; ++i)
    {
      // x,y here avoids right to left function evaluation
      // result.push_back(Point64(*v++, *v++));
      int64_t x = *v++;
      int64_t y = *v++;
      result.push_back(Point64(x, y));
    }
  }
  return result;
}

inline CPaths64 CreateCPaths64(const Paths64& pp)
{
  size_t cnt = pp.size(), cnt2 = cnt;
  // don't allocate space for empty paths
  for (size_t i = 0; i < cnt; ++i)
    if (!pp[i].size()) --cnt2;
  if (!cnt2) return nullptr;

  CPaths64 result = new int64_t* [cnt2 + 1];
  CPaths64 v = result;
  *v++ = CreateCPath64(0, cnt2); // assign a counter path
  for (const Path64& p : pp)
  {
    *v = CreateCPath64(p);
    if (*v) ++v;
  }
  return result;
}

inline Paths64 ConvertCPaths64(const CPaths64& pp)
{
  Paths64 result;
  if (pp) 
  {
    CPaths64 v = pp;
    CPath64 cnts = pp[0];
    const size_t cnt = static_cast<size_t>(cnts[1]); // nb 2nd cnt
    ++v; // skip cnts
    result.reserve(cnt);
    for (size_t i = 0; i < cnt; ++i)
      result.push_back(ConvertCPath64(*v++));
  }
  return result;
}

inline CPathD CreateCPathD(size_t cnt1, size_t cnt2)
{
  // create a dummy path counter
  CPathD result = new double[2 + cnt1 * 2];
  result[0] = static_cast<double>(cnt1);
  result[1] = static_cast<double>(cnt2);
  return result;
}

inline CPathD CreateCPathD(const PathD& p)
{
  size_t cnt = p.size();
  if (!cnt) return nullptr; 
  CPathD result = CreateCPathD(cnt, 0);
  CPathD v = result;
  v += 2; // skip counters
  for (const PointD& pt : p)
  {
    *v++ = pt.x;
    *v++ = pt.y;
  }
  return result;
}

inline PathD ConvertCPathD(const CPathD& p)
{
  PathD result;
  if (p)
  {
    CPathD v = p;
    size_t cnt = static_cast<size_t>(v[0]);
    v += 2; // skip counters
    result.reserve(cnt);
    for (size_t i = 0; i < cnt; ++i)
    {
      // x,y here avoids right to left function evaluation
      // result.push_back(PointD(*v++, *v++));
      double x = *v++;
      double y = *v++;
      result.push_back(PointD(x, y));
    }
  }
  return result;
}

inline CPathsD CreateCPathsD(const PathsD& pp)
{
  size_t cnt = pp.size(), cnt2 = cnt;
  // don't allocate space for empty paths
  for (size_t i = 0; i < cnt; ++i)
    if (!pp[i].size()) --cnt2;
  if (!cnt2) return nullptr;
  CPathsD result = new double * [cnt2 + 1];
  CPathsD v = result;
  *v++ = CreateCPathD(0, cnt2); // assign counter path
  for (const PathD& p : pp)
  {
    *v = CreateCPathD(p);
    if (*v) { ++v; }
  }
  return result;
}

inline PathsD ConvertCPathsD(const CPathsD& pp)
{
  PathsD result;
  if (pp)
  {
    CPathsD v = pp;
    CPathD cnts = v[0];
    size_t cnt = static_cast<size_t>(cnts[1]);
    ++v; // skip cnts path
    result.reserve(cnt);
    for (size_t i = 0; i < cnt; ++i)
      result.push_back(ConvertCPathD(*v++));
  }
  return result;
}

inline Path64 ConvertCPathD(const CPathD& p, double scale)
{
  Path64 result;
  if (p)
  {
    CPathD v = p;
    size_t cnt = static_cast<size_t>(*v);
    v += 2; // skip counters
    result.reserve(cnt);
    for (size_t i = 0; i < cnt; ++i)
    {
      // x,y here avoids right to left function evaluation
      // result.push_back(PointD(*v++, *v++));
      double x = *v++ * scale;
      double y = *v++ * scale;
      result.push_back(Point64(x, y));
    }
  }
  return result;
}

inline Paths64 ConvertCPathsD(const CPathsD& pp, double scale)
{
  Paths64 result;
  if (pp)
  {
    CPathsD v = pp;
    CPathD cnts = v[0];
    size_t cnt = static_cast<size_t>(cnts[1]);
    result.reserve(cnt);
    ++v; // skip cnts path
    for (size_t i = 0; i < cnt; ++i)
      result.push_back(ConvertCPathD(*v++, scale));
  }
  return result;
}

inline CPathD CreateCPathD(const Path64& p, double scale)
{
  size_t cnt = p.size();
  if (!cnt) return nullptr;
  CPathD result = CreateCPathD(cnt, 0);
  CPathD v = result;
  v += 2; // skip cnts 
  for (const Point64& pt : p)
  {
    *v++ = pt.x * scale;
    *v++ = pt.y * scale;
  }
  return result;
}

inline CPathsD CreateCPathsD(const Paths64& pp, double scale)
{
  size_t cnt = pp.size(), cnt2 = cnt;
  // don't allocate space for empty paths
  for (size_t i = 0; i < cnt; ++i)
    if (!pp[i].size()) --cnt2;
  if (!cnt2) return nullptr;
  CPathsD result = new double* [cnt2 + 1];
  CPathsD v = result;
  *v++ = CreateCPathD(0, cnt2);
  for (const Path64& p : pp)
  {
    *v = CreateCPathD(p, scale);
    if (*v) ++v;
  }
  return result;
}

inline void InitCPolyPath64(CPolyTree64* cpt, 
  bool is_hole, const PolyPath64* pp)
{
  cpt->polygon = CreateCPath64(pp->Polygon());
  cpt->is_hole = is_hole;
  size_t child_cnt = pp->Count();
  cpt->child_count = child_cnt;
  cpt->childs = nullptr;
  if (!child_cnt) return;
  cpt->childs = new CPolyPath64[child_cnt];
  CPolyPath64* child = cpt->childs;
  for (const PolyPath64* pp_child : *pp)
    InitCPolyPath64(child++, !is_hole, pp_child);  
}

inline CPolyTree64* CreateCPolyTree64(const PolyTree64& pt)
{
  CPolyTree64* result = new CPolyTree64();
  result->polygon = nullptr;
  result->is_hole = false;
  size_t child_cnt = pt.Count();
  result->childs = nullptr;
  result->child_count = child_cnt;
  if (!child_cnt) return result;
  result->childs = new CPolyPath64[child_cnt];
  CPolyPath64* child = result->childs;
  for (const PolyPath64* pp : pt)
    InitCPolyPath64(child++, true, pp);
  return result;
}

inline void DisposeCPolyPath64(CPolyPath64* cpp) 
{
  if (!cpp->child_count) return;
  CPolyPath64* child = cpp->childs;
  for (size_t i = 0; i < cpp->child_count; ++i)
    DisposeCPolyPath64(child);
  delete[] cpp->childs;
}

EXTERN_DLL_EXPORT void DisposeExportedCPolyTree64(CPolyTree64*& cpt)
{
  if (!cpt) return;
  DisposeCPolyPath64(cpt);
  delete cpt;
  cpt = nullptr;
}

inline void InitCPolyPathD(CPolyTreeD* cpt,
  bool is_hole, const PolyPath64* pp, double scale)
{
  cpt->polygon = CreateCPathD(pp->Polygon(), scale);
  cpt->is_hole = is_hole;
  size_t child_cnt = pp->Count();
  cpt->child_count = child_cnt;
  cpt->childs = nullptr;
  if (!child_cnt) return;
  cpt->childs = new CPolyPathD[child_cnt];
  CPolyPathD* child = cpt->childs;
  for (const PolyPath64* pp_child : *pp)
    InitCPolyPathD(child++, !is_hole, pp_child, scale);
}

inline CPolyTreeD* CreateCPolyTreeD(const PolyTree64& pt, double scale)
{
  CPolyTreeD* result = new CPolyTreeD();
  result->polygon = nullptr;
  result->is_hole = false;
  size_t child_cnt = pt.Count();
  result->child_count = child_cnt;
  result->childs = nullptr;
  if (!child_cnt) return result;
  result->childs = new CPolyPathD[child_cnt];
  CPolyPathD* child = result->childs;
  for (const PolyPath64* pp : pt)
    InitCPolyPathD(child++, true, pp, scale);
  return result;
}

inline void DisposeCPolyPathD(CPolyPathD* cpp)
{
  if (!cpp->child_count) return;
  CPolyPathD* child = cpp->childs;
  for (size_t i = 0; i < cpp->child_count; ++i)
    DisposeCPolyPathD(child++);
  delete[] cpp->childs;
}

EXTERN_DLL_EXPORT void DisposeExportedCPolyTreeD(CPolyTreeD*& cpt)
{
  if (!cpt) return;
  DisposeCPolyPathD(cpt);
  delete cpt;
  cpt = nullptr;
}

}  // end Clipper2Lib namespace
  
#endif  // CLIPPER2_EXPORT_H
