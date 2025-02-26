 unit SvgWriter;

(*******************************************************************************
* Author    :  Angus Johnson                                                   *
* Date      :  26 October 2022                                                 *
* Website   :  http://www.angusj.com                                           *
* Copyright :  Angus Johnson 2010-2022                                         *
* Purpose   :  This module provides a very simple SVG Writer                   *
* License   :  http://www.boost.org/LICENSE_1_0.txt                            *
*******************************************************************************)

interface

{$I ..\Clipper2Lib\Clipper.inc}

uses
  Classes,
  {$IFDEF USINGCLIPPER2LIB}
  Clipper,
  {$ENDIF}
  SysUtils, Math;

const
  black   = $FF000000;
  white   = $FFFFFFFF;
  maroon  = $FF800000;
  navy    = $FF000080;
  blue    = $FF0000FF;
  red     = $FFFF0000;
  green   = $FF008000;
  yellow  = $FFFFFF00;
  lime    = $FF00FF00;
  fuscia  = $FFFF00FF;
  aqua    = $FF00FFFF;

type

{$IFNDEF USINGCLIPPER2LIB}
  TFillRule = (frEvenOdd, frNonZero, frPositive, frNegative);
  TPointD = record X,Y: double; end;
  TPathD = array of TPointD;
  TPathsD = array of TPathD;
  TRectD = record left,top,right,bottom: double; end;
  TPoint64 = record X,Y: Int64; end;
  TPath64 = array of TPoint64;
  TPaths64 = array of TPath64;
  TRect64 = record left,top,right,bottom: double; end;
{$ENDIF}

  PPointD = ^TPointD;
  TArrayOfInteger = array of Integer;

{$IFDEF RECORD_METHODS}
  TCoordStyle = record
{$ELSE}
  TCoordStyle = object
{$ENDIF}
    FontName: string;
    FontSize: integer;
    FontColor: cardinal;
    constructor Create(const afontname: string;
      afontsize: integer; afontcolor: Cardinal);
  end;

  PTextInfo = ^TTextInfo;
{$IFDEF RECORD_METHODS}
  TTextInfo = record
{$ELSE}
  TTextInfo = object
{$ENDIF}
    x,y : integer;
    text: string;
    fontSize: integer;
    fontColor: Cardinal;
    Bold: Boolean;
    constructor Create(atext: string; _x, _y: integer;
      afontsize: integer = 12;
      afontcolor: Cardinal = black;
      aBold: Boolean = false);
  end;

  PPolyInfo = ^TPolyInfo;
  TPolyInfo = record
    paths     : TPathsD;
    BrushClr  : Cardinal;
    PenClr    : Cardinal;
    PenWidth  : double;
    ShowCoords: Boolean;
    IsOpen    : Boolean;
    dashes    : TArrayOfInteger;
  end;

  PCircleInfo = ^TCircleInfo;
  TCircleInfo = record
    center    : TPointD;
    radius    : double;
    BrushClr  : Cardinal;
    PenClr    : Cardinal;
    PenWidth  : double;
  end;

  TSvgWriter = class
  private
    fFillRule   : TFillRule;
    fCoordStyle : TCoordStyle;
    fPolyInfos  : TList;
    fCircleInfos: TList;
    fTextInfos  : TList;
    function GetBounds: TRectD;
  public
    constructor Create(fillRule: TFillRule;
      const coordFontName: string = 'Verdana';
      coordFontSize: integer = 9;
      coordFontColor: Cardinal = black);
    destructor Destroy; override;
    procedure AddPaths(const paths: TPaths64; isOpen: Boolean;
      brushColor, penColor: Cardinal;
      penWidth: double; showCoords: Boolean = false); overload;
    procedure AddPaths(const paths: TPathsD; isOpen: Boolean;
      brushColor, penColor: Cardinal;
      penWidth: double; showCoords: Boolean = false); overload;
    procedure AddDashedPath(const paths: TPathsD; penColor: Cardinal;
      penWidth: double; const dashes: TArrayOfInteger);

    procedure AddArrow(const center: TPointD;
      radius: double; angleRad: double;
      brushColor, penColor: Cardinal;
      penWidth: double); overload;
    procedure AddCircle(const center: TPointD; radius: double;
      brushColor, penColor: Cardinal; penWidth: double); overload;
    procedure AddText(text: string; x,y: integer;
      fontSize: integer = 14; fontClr:
      Cardinal = black; bold: Boolean = false);
    function SaveToFile(const filename: string;
      maxWidth: integer = 0; maxHeight: integer = 0;
      margin: integer = 20): Boolean;
    procedure ClearPaths;
    procedure ClearText;
    procedure ClearAll;
  end;

  procedure AddSubject(svg: TSvgWriter; const paths: TPaths64); overload;
  procedure AddOpenSubject(svg: TSvgWriter; const paths: TPaths64); overload;
  procedure AddClip(svg: TSvgWriter; const paths: TPaths64); overload;
  procedure AddSolution(svg: TSvgWriter; const paths: TPaths64); overload;
  procedure AddOpenSolution(svg: TSvgWriter; const paths: TPaths64); overload;
  procedure AddSubject(svg: TSvgWriter; const paths: TPathsD); overload;
  procedure AddOpenSubject(svg: TSvgWriter; const paths: TPathsD); overload;
  procedure AddClip(svg: TSvgWriter; const paths: TPathsD); overload;
  procedure AddSolution(svg: TSvgWriter; const paths: TPathsD); overload;
  procedure AddOpenSolution(svg: TSvgWriter; const paths: TPathsD); overload;

  procedure SaveSvg(svg: TSvgWriter; const filename: string;
    width: integer = 0; height: integer = 0; margin: integer = 0);


implementation

const
  MaxRect: TRectD  = (left: MaxDouble;
    Top: MaxDouble; Right: -MaxDouble; Bottom: -MaxDouble);
  NullRectD   : TRectD = (left: 0; top: 0; right: 0; Bottom: 0);

  svg_header: string =
      '<svg width="%dpx" height="%dpx" viewBox="0 0 %0:d %1:d" ' +
      'version="1.1" xmlns="http://www.w3.org/2000/svg">';
  svg_path_format: string = '"'+#10+'    style="fill:%s;' +
        ' fill-opacity:%1.2f; fill-rule:%s; stroke:%s;' +
        ' stroke-opacity:%1.2f; stroke-width:%1.2f;"/>'#10;
  svg_path_format2: string = '"'+#10+'    style="fill:none; stroke:%s; ' +
        'stroke-opacity:%1.2f; stroke-width:%1.2f; %s"/>'#10;

function GetBounds(const paths: TPathsD): TRectD;
var
  i, j: Integer;
  p: PPointD;
begin
  Result.Left :=infinity;
  Result.Top := infinity;
  Result.Right := -infinity;
  Result.Bottom := -infinity;

  for i := 0 to High(paths) do
    if Assigned(paths[i]) then
    begin
      p := @paths[i][0];
      for j := 0 to High(paths[i]) do
      begin
        if p.X < Result.Left then Result.Left := p.X;
        if p.X > Result.Right then Result.Right := p.X;
        if p.Y < Result.Top then Result.Top := p.Y;
        if p.Y > Result.Bottom then Result.Bottom := p.Y;
        inc(p);
      end;
    end;
  if Result.Left >= Result.Right then Result := nullRectD;
end;

function PathD(const path: TPath64): TPathD;
var
  i, len: integer;
begin
  len := Length(path);
  setLength(Result, len);
  for i := 0 to len -1 do
  begin
    Result[i].X := path[i].X;
    Result[i].Y := path[i].Y;
  end;
end;

function PathsD(const paths: TPaths64): TPathsD;
var
  i, len: integer;
begin
  len := Length(paths);
  setLength(Result, len);
  for i := 0 to len -1 do
    Result[i] := PathD(paths[i]);
end;
//------------------------------------------------------------------------------

function Ellipse(l,t,r,b: double; steps: integer): TPathD;
var
  i: Integer;
  x, sinA, cosA: double;
  centre, radius, delta: TPointD;
begin
  result := nil;
  if (r <= l) or (b <= t) then Exit;
  centre.X := (r + l) * 0.5;
  centre.Y := (b + t) * 0.5;
  radius.X := (r - l) * 0.5;
  radius.Y := (b - l) * 0.5;
  if (steps < 3) then
    steps := Ceil(PI * sqrt((radius.X + radius.Y) * 2));
  SinCos(2 * Pi / Steps, sinA, cosA);
  delta.x := cosA; delta.y := sinA;
  SetLength(Result, Steps);
  Result[0].X := centre.X + radius.X;
  Result[0].y := centre.Y;
  for i := 1 to steps -1 do
  begin

    Result[i].X := centre.X + radius.X * delta.x;
    Result[i].Y := centre.Y + radius.y * delta.y;
    x := delta.X * cosA - delta.Y * sinA;
    delta.X :=  x;
    delta.Y := delta.Y * cosA + x * sinA;
  end; // rotates clockwise
end;

procedure RotatePt(var pt: TPointD; const center: TPointD; sinA, cosA: double);
var
  tmpX, tmpY: double;
begin
  tmpX := pt.X-center.X;
  tmpY := pt.Y-center.Y;
  pt.X := tmpX * cosA - tmpY * sinA + center.X;
  pt.Y := tmpX * sinA + tmpY * cosA + center.Y;
end;

procedure RotatePath(var path: TPathD;
  const center: TPointD; sinA, cosA: double);
var
  i: integer;
begin
  for i := 0 to High(path) do
    RotatePt(path[i], center, sinA, cosA);
end;
//------------------------------------------------------------------------------


function ColorToHtml(color: Cardinal): string;
begin
  Result := Format('#%6.6x', [color and $FFFFFF]);;
end;

function GetAlpha(clr: Cardinal): double;
begin
  Result := (clr shr 24) / 255;
end;

constructor TCoordStyle.Create(const afontname: string;
  afontsize: integer; afontcolor: Cardinal);
begin
  Self.FontName   := afontname;
  Self.FontSize   := afontsize;
  Self.FontColor  := afontcolor;
end;

constructor TTextInfo.Create(atext: string; _x, _y: integer;
  afontsize: integer = 12; afontcolor: Cardinal = black;
  aBold: Boolean = false);
begin
  self.x := _x;
  self.y := _y;
  self.text := text;
  self.fontSize := afontsize;
  self.fontColor := afontcolor;
  Self.Bold       := aBold;
end;

constructor TSvgWriter.Create(fillRule: TFillRule;
  const coordFontName: string = 'Verdana';
  coordFontSize: integer = 9;
  coordFontColor: Cardinal = black);
begin
  fFillRule := fillRule;
  fCoordStyle.FontName := coordFontName;
  fCoordStyle.FontSize := coordFontSize;
  fCoordStyle.FontColor := coordFontColor;
  fPolyInfos := TList.Create;
  fCircleInfos := TList.Create;
  fTextInfos := TList.Create;
end;

destructor TSvgWriter.Destroy;
begin
  ClearAll;
  fPolyInfos.Free;
  fCircleInfos.Free;
  fTextInfos.Free;
  inherited;
end;

procedure TSvgWriter.AddPaths(const paths: TPaths64; isOpen: Boolean;
  brushColor, penColor: Cardinal;
  penWidth: double; showCoords: Boolean = false);
var
  pi: PPolyInfo;
begin
  if Length(paths) = 0 then Exit;
  new(pi);
  pi.paths := PathsD(paths);
  pi.BrushClr := brushColor;
  pi.PenClr   := penColor;
  pi.PenWidth := penWidth;
  pi.ShowCoords := showCoords;
  pi.IsOpen := isOpen;
  fPolyInfos.Add(pi);
end;

procedure TSvgWriter.AddPaths(const paths: TPathsD; isOpen: Boolean;
  brushColor, penColor: Cardinal;
  penWidth: double; showCoords: Boolean = false);
var
  pi: PPolyInfo;
begin
  new(pi);
  pi.paths := Copy(paths, 0, Length(paths));
  pi.BrushClr := brushColor;
  pi.PenClr   := penColor;
  pi.PenWidth := penWidth;
  pi.ShowCoords := showCoords;
  pi.IsOpen := isOpen;
  fPolyInfos.Add(pi);
end;

procedure TSvgWriter.AddDashedPath(const paths: TPathsD;
  penColor: Cardinal; penWidth: double; const dashes: TArrayOfInteger);
var
  pi: PPolyInfo;
begin
  new(pi);
  pi.paths := Copy(paths, 0, Length(paths));
  pi.BrushClr := 0;
  pi.PenClr   := penColor;
  pi.PenWidth := penWidth;
  pi.ShowCoords := false;
  pi.IsOpen := true;
  pi.dashes := Copy(dashes, 0, Length(dashes));
  fPolyInfos.Add(pi);
end;

procedure TSvgWriter.AddArrow(const center: TPointD;
  radius: double; angleRad: double; brushColor, penColor: Cardinal;
  penWidth: double);
var
  pp: TPathsD;
  s,c: double;
begin
  SetLength(pp, 1);
  with center do
    pp[0] := Ellipse(X - radius * 1.2, Y - radius * 0.9,
      X + radius * 1.2, Y + radius * 0.9, 3);
  if angleRad <> 0 then
  begin
    SinCos(angleRad, s,c);
    RotatePath(pp[0], center, s, c);
  end;
  AddPaths(pp, false, brushColor, penColor, penWidth);
end;

procedure TSvgWriter.AddCircle(const center: TPointD;
  radius: double; brushColor, penColor: Cardinal; penWidth: double);
var
  ci: PCircleInfo;
begin
  new(ci);
  ci.center := center;
  ci.radius := radius;
  ci.BrushClr := brushColor;
  ci.PenClr   := penColor;
  ci.PenWidth := penWidth;
  fCircleInfos.Add(ci);
end;

procedure TSvgWriter.AddText(text: string; x,y: integer;
  fontSize: integer; fontClr: Cardinal; bold: Boolean);
var
  ti: PTextInfo;
begin
  new(ti);
  ti.x := x;
  ti.y := y;
  ti.text := text;
  ti.fontSize := fontSize;
  ti.fontColor := fontClr;
  ti.Bold := bold;
  fTextInfos.Add(ti);
end;

function TSvgWriter.GetBounds: TRectD;
var
  i: integer;
  bounds: TRectD;
begin
  Result := MaxRect;
  for i := 0 to fPolyInfos.Count -1 do
    with PPolyInfo(fPolyInfos[i])^ do
    begin
      bounds := SvgWriter.GetBounds(paths);
      if (bounds.left < Result.Left) then Result.Left := bounds.Left;
      if (bounds.right> Result.Right) then Result.Right := bounds.Right;
      if (bounds.top < Result.Top) then Result.Top := bounds.Top;
      if (bounds.bottom > Result.Bottom) then Result.Bottom := bounds.Bottom;
    end;
end;

procedure TSvgWriter.ClearPaths;
var
  i: integer;
begin
  for i := 0 to fPolyInfos.Count -1 do
    Dispose(PPolyInfo(fPolyInfos[i]));
  fPolyInfos.Clear;

  for i := 0 to fCircleInfos.Count -1 do
    Dispose(PCircleInfo(fCircleInfos[i]));
  fCircleInfos.Clear;
end;

procedure TSvgWriter.ClearText;
var
  i: integer;
begin
  for i := 0 to fTextInfos.Count -1 do
    Dispose(PTextInfo(fTextInfos[i]));
  fTextInfos.Clear;
end;

procedure TSvgWriter.ClearAll;
begin
  ClearText;
  ClearPaths;
end;

function TSvgWriter.SaveToFile(const filename: string;
  maxWidth: integer = 0; maxHeight: integer = 0; margin: integer = 20): Boolean;
var
  i,j,k: integer;
  bounds: TRectD;
  scale: double;
  offsetX, offsetY: integer;
  s, sInline, dashStr: string;
  sl: TStringList;
  formatSettings: TFormatSettings;
const
  fillRuleStr: array[boolean] of string = ('evenodd', 'nonzero');
  boldFont: array[boolean] of string = ('normal', '700');

  procedure Add(const s: string);
  begin
    sl.Add( sInline + s);
    sInline := '';
  end;

  procedure AddInline(const s: string);
  begin
    sInline := sInline + s;
  end;

begin
  formatSettings := TFormatSettings.Create;
  formatSettings.DecimalSeparator := '.';

  Result := false;
  if (margin < 20) then margin := 20;
  bounds := GetBounds;
  if (bounds.Right <= bounds.left) or
    (bounds.Bottom <= bounds.Top) then Exit;

  scale := 1.0;
  if (maxWidth > 0) and (maxHeight > 0) then
    scale := 1.0 / Max((bounds.right - bounds.left) /
      (maxWidth - margin * 2), (bounds.bottom - bounds.top) /
      (maxHeight - margin * 2));

  offsetX := margin - Round(bounds.left * scale);
  offsetY := margin - Round(bounds.top * scale);

  sl := TStringList.Create;
  try
    if (maxWidth <= 0) or (maxHeight <= 0) then
       Add(Format(svg_header,
       [Round(bounds.right - bounds.left) + margin * 2,
        Round(bounds.bottom - bounds.top) + margin * 2], formatSettings))
    else
      Add(Format(svg_header, [maxWidth, maxHeight], formatSettings));

    for i := 0 to fPolyInfos.Count -1 do
      with PPolyInfo(fPolyInfos[i])^ do
      begin
        AddInline('  <path d="');
        for j := 0 to High(paths) do
        begin
          if Length(paths[j]) < 2 then Continue;
          if not IsOpen and (Length(paths[j]) < 3) then Continue;
          AddInline(Format('M %1.2f %1.2f L ',
            [paths[j][0].x * scale + offsetX,
            paths[j][0].y * scale + offsetY], formatSettings));
          for k := 1 to High(paths[j]) do
            AddInline(Format('%1.2f %1.2f ',
              [paths[j][k].x * scale + offsetX,
              paths[j][k].y * scale + offsetY], formatSettings));
          if not IsOpen then AddInline('Z');
        end;

        if not IsOpen then
          Add(Format(svg_path_format,
            [ColorToHtml(BrushClr), GetAlpha(BrushClr),
            fillRuleStr[fFillRule = frNonZero],
            ColorToHtml(PenClr), GetAlpha(PenClr), PenWidth], formatSettings))
        else
        begin
          dashStr := '';;
          if Length(dashes) > 0 then
          begin
            s := '';
            for k := 0 to High(dashes) do
              s := s + IntToStr(dashes[k]) + ' ';
            dashStr := 'stroke-dasharray: ' + s + ';';
          end;
          Add(Format(svg_path_format2,
              [ColorToHtml(PenClr), GetAlpha(PenClr),
              PenWidth, dashStr], formatSettings));
        end;

        if (ShowCoords) then
        begin
          with fCoordStyle do
            Add(Format('<g font-family="%s" font-size="%d" fill="%s">',
              [FontName, FontSize, ColorToHtml(FontColor)], formatSettings));
          for j := 0 to High(paths) do
            for k := 0 to High(paths[j]) do
              with paths[j][k] do
                Add(Format('  <text x="%1.2f" y="%1.2f">%1.0f,%1.0f</text>',
                  [x * scale + offsetX, y * scale + offsetY, x, y],
                  formatSettings));
          Add('</g>'#10);
        end;
      end;

      for i := 0 to fCircleInfos.Count -1 do
        with PCircleInfo(fCircleInfos[i])^ do
        begin
          Add(Format('  <circle cx="%1.2f" cy="%1.2f" r="%1.2f" '+
            'stroke="%s" stroke-width="%1.2f" fill="%s" />',
            [center.X * scale + offsetX, center.Y * scale + offsetY,
            radius, ColorToHtml(PenClr),
            PenWidth, ColorToHtml(BrushClr)], formatSettings));
        end;

    for i := 0 to fTextInfos.Count -1 do
      with PTextInfo(fTextInfos[i])^ do
      begin
            Add(Format(
            '  <g font-family="Verdana" font-style="normal" ' +
            'font-weight="%s" font-size="%d" fill="%s">' +
            '<text x="%1.2f" y="%1.2f">%s</text></g>',
            [boldFont[Bold], Round(fontSize * scale),
            ColorToHtml(fontColor),
            (x) * scale + offsetX,
            (y) * scale + offsetY, text], formatSettings));
      end;
    Add('</svg>'#10);
    sl.SaveToFile(filename);
    Result := true;
  finally
    sl.free;
  end;
end;

procedure AddSubject(svg: TSvgWriter; const paths: TPaths64);
begin
  svg.AddPaths(paths, false, $200099FF, $800066FF, 1.0);
end;

procedure AddOpenSubject(svg: TSvgWriter; const paths: TPaths64);
begin
  svg.AddPaths(paths, true, $0, $800066FF, 2.2);
end;

procedure AddClip(svg: TSvgWriter; const paths: TPaths64);
begin
  svg.AddPaths(paths, false, $10FF9900, $80FF6600, 1.0);
end;

procedure AddSolution(svg: TSvgWriter; const paths: TPaths64);
begin
  svg.AddPaths(paths, false, $8066FF66, $FF006600, 1.5);
end;

procedure AddOpenSolution(svg: TSvgWriter; const paths: TPaths64);
begin
  svg.AddPaths(paths, false, $8066FF66, $FF006600, 1.5);
end;


procedure AddSubject(svg: TSvgWriter; const paths: TPathsD); overload;
begin
  svg.AddPaths(paths, false, $200099FF, $800066FF, 1.0);
end;

procedure AddOpenSubject(svg: TSvgWriter; const paths: TPathsD); overload;
begin
  svg.AddPaths(paths, true, $0, $800066FF, 2.2);
end;

procedure AddClip(svg: TSvgWriter; const paths: TPathsD); overload;
begin
  svg.AddPaths(paths, false, $10FF9900, $80FF6600, 1.0);
end;

procedure AddSolution(svg: TSvgWriter; const paths: TPathsD); overload;
begin
  svg.AddPaths(paths, false, $8066FF66, $FF006600, 1.5);
end;

procedure AddOpenSolution(svg: TSvgWriter; const paths: TPathsD); overload;
begin
  svg.AddPaths(paths, false, $8066FF66, $FF006600, 1.5);
end;

procedure SaveSvg(svg: TSvgWriter; const filename: string;
  width: integer = 0; height: integer = 0; margin: integer = 0);
begin
  svg.SaveToFile(filename, width, height, margin);
end;

end.

