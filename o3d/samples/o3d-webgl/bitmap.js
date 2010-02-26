/*
 * Copyright 2010, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/**
 * Bitmap provides an interface for basic image operations on bitmap,
 * including scale and crop. A Bitmap can be created from RawData via
 * pack.createBitmapsFromRawData(), and also can be transferred to mip of a
 * Texure2D or a specific face of TextureCUBE via methods in Texture.
 * @constructor
 */
o3d.Bitmap = function() {
  o3d.ParamObject.call(this);
};
o3d.inherit('Bitmap', 'ParamObject');


/**
 * @type {number}
 */
o3d.Bitmap.Semantic = goog.typedef;

/**
 * After loading an array of Bitmaps with pack.createBitmapsFromRawData
 * you can inspect their semantic to see what they were intended for. This is
 * mostly to distinguish between 6 bitmaps that are faces of a cubemap and 6
 * bitmaps that are slices of a 3d texture.
 * 
 *  FACE_POSITIVE_X, 1 face of a cubemap
 *  FACE_NEGATIVE_X, 1 face of a cubemap
 *  FACE_POSITIVE_Y, 1 face of a cubemap
 *  FACE_NEGATIVE_Y, 1 face of a cubemap
 *  FACE_POSITIVE_Z, 1 face of a cubemap
 *  FACE_NEGATIVE_Z, 1 face of a cubemap
 *  IMAGE, normal 2d image
 *  SLICE, a slice of a 3d texture.
 */
o3d.Bitmap.FACE_POSITIVE_X = 0;
o3d.Bitmap.FACE_NEGATIVE_X = 1;
o3d.Bitmap.FACE_POSITIVE_Y = 2;
o3d.Bitmap.FACE_NEGATIVE_Y = 3;
o3d.Bitmap.FACE_POSITIVE_Z = 4;
o3d.Bitmap.FACE_NEGATIVE_Z = 5;
o3d.Bitmap.IMAGE = 6;
o3d.Bitmap.SLICE = 7;



/**
 * Flips a bitmap vertically in place.
 * @type {boolean}
 */
o3d.Bitmap.prototype.flipVertically = false;



/**
 * Generates mip maps from the source level to lower levels.
 * 
 * You can not currently generate mips for DXT textures although you can load
 * them from dds files.
 * 
 * @param {number} source_level The level to use as the source of the mips.
 * @param {number} num_levels The number of levels to generate below the
 *     source level.
 */
o3d.Bitmap.prototype.generateMips =
    function(source_level, num_levels) {
  o3d.notImplemented();
};


/**
 * The width of the bitmap (read only).
 * @type {number}
 */
o3d.Bitmap.prototype.width = 0;



/**
 * The height of the bitmap (read only).
 * @type {number}
 */
o3d.Bitmap.prototype.height = 0;



/**
 * The format of the bitmap (read only).
 * @type {number}
 */
o3d.Bitmap.prototype.format = 0;



/**
 * Number mip-maps (read only)
 * @type {number}
 */
o3d.Bitmap.prototype.numMipmaps = 1;



/**
 * The Semantic of the bitmap.
 * @type {!o3d.Stream.Semantic}
 */
o3d.Bitmap.prototype.semantic = o3d.Stream.UNKNOWN_SEMANTIC;



