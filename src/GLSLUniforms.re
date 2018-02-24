type uniformInputT = {
  gl: WebGL2.glT,
  time: float,
  tick: float,
  width: int,
  height: int,
  modelViewMatrix: array(float),
  projectionMatrix: array(float)
};

/*
 type uniformTextureT = {
    wrapS: WebGL2.glT => WebGL2.clampT,
    wrapT: WebGL2.glT => WebGL2.clampT,
   name: string,
   texture: WebGL2.glT => WebGL2.textureT
 };
    */
type uniformFunctionT =
  | UniformFloatArray(uniformInputT => array(float))
  | UniformTexture(string, uniformInputT => WebGL2.textureT);

type uniformBlockT = list((GLSL.rT, uniformInputT => array(float)));

let registerUniform = (a, b) => (GLSL.untyped(a), UniformFloatArray(b));

let registerTextureUniform = (a, b) => {
  let a = GLSL.untyped(a);
  let name =
    switch a {
    | GLSL.RVar((_, _, name)) => name
    | _ => raise(GLSL.GLSLTypeError("bad uniform"))
    };
  (a, UniformTexture(name, b));
};

let cachedEmptyTexture = ref(None);

let emptyTexture = gl => {
  let retval =
    switch cachedEmptyTexture^ {
    | Some(x) => x
    | None => WebGL2.createTexture(gl)
    };
  retval;
};

let setupTexture = (gl, texture) => {
  let t2d = WebGL2.getTEXTURE_2D(gl);
  WebGL2.bindTexture(gl, t2d, texture);
  WebGL2.texParameteri(
    gl,
    t2d,
    WebGL2.getTEXTURE_WRAP_S(gl),
    WebGL2.getCLAMP_TO_EDGE(gl)
  );
  WebGL2.texParameteri(
    gl,
    t2d,
    WebGL2.getTEXTURE_WRAP_T(gl),
    WebGL2.getCLAMP_TO_EDGE(gl)
  );
  WebGL2.texParameteri(
    gl,
    t2d,
    WebGL2.getTEXTURE_MIN_FILTER(gl),
    WebGL2.getNEAREST(gl)
  );
  WebGL2.texParameteri(
    gl,
    t2d,
    WebGL2.getTEXTURE_MAG_FILTER(gl),
    WebGL2.getNEAREST(gl)
  );
};

let uploadImage = (gl, texture, img) => {
  let t2d = WebGL2.getTEXTURE_2D(gl);
  WebGL2.bindTexture(gl, t2d, texture);
  /* the largest mip */
  let mipLevel = 0;
  /* format we want in the texture */
  let internalFormat = WebGL2.getRGBA(gl);
  /* format of data we are supplying */
  let srcFormat = WebGL2.getRGBA(gl);
  /* type of data we are supplying */
  let srcType = WebGL2.getUNSIGNED_BYTE(gl);
  /* Upload the image into the texture. */
  WebGL2.texImage2D(
    gl,
    WebGL2.getTEXTURE_2D(gl),
    mipLevel,
    internalFormat,
    srcFormat,
    srcType,
    img
  );
};

/* TODO: Preallocate and refill array */
let computeUniformBlock =
    (gl, time, width, height, modelViewMatrix, projectionMatrix, uniforms) => {
  let uniformArg = {
    gl,
    time,
    tick: 0.0,
    width,
    height,
    modelViewMatrix,
    projectionMatrix
  };
  let alignAt2 = l =>
    switch (Array.length(l) mod 2) {
    | 0 => l
    | 1 => Array.concat([l, [|0.0|]])
    | _ => raise(GLSL.GLSLTypeError("can't happen"))
    };
  let alignAt4 = l =>
    switch (Array.length(l) mod 4) {
    | 0 => l
    | 1 => Array.concat([l, [|0.0, 0.0, 0.0|]])
    | 2 => Array.concat([l, [|0.0, 0.0|]])
    | 3 => Array.concat([l, [|0.0|]])
    | _ => raise(GLSL.GLSLTypeError("can't happen"))
    };
  let l =
    List.fold_left(
      (ar, (u, f)) => {
        let t =
          switch u {
          | GLSL.RVar((_, t2, _)) => t2
          | _ => raise(GLSL.GLSLTypeError("not rvar"))
          };
        /** Layout std140 according to
         * https://www.khronos.org/registry/OpenGL/specs/gl/glspec45.core.pdf#page=159
         * */
        let align = x =>
          switch t {
          | GLSL.Void => x
          | GLSL.Int => x
          | GLSL.Float => x
          | GLSL.Vec2 => alignAt2(x)
          | GLSL.Vec3 => alignAt4(x)
          | GLSL.Vec4 => alignAt2(x)
          | GLSL.Mat2 => x
          | GLSL.Mat3 => x
          | GLSL.Mat4 => x
          | GLSL.Sampler2D => x
          | GLSL.SamplerCube => x
          };
        switch f {
        | UniformFloatArray(f2) => Array.concat([align(ar), f2(uniformArg)])
        | _ => ar
        };
      },
      [||],
      uniforms
    );
  let uniformBlock = Float32Array.create(l);
  let textures =
    List.fold_right(
      ((_, f), r) =>
        switch f {
        | UniformTexture(name, f2) => [(name, f2(uniformArg)), ...r]
        | _ => r
        },
      uniforms,
      []
    );
  (uniformBlock, textures);
};
