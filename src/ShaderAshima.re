/*
 // Description : Array and textureless GLSL 2D simplex noise function.
 //      Author : Ian McEwan, Ashima Arts.
 //  Maintainer : stegu
 //     Lastmod : 20110822 (ijm)
 //     License : Copyright (C) 2011 Ashima Arts. All rights reserved.
 //               Distributed under the MIT License. See LICENSE file.
 //               https://github.com/ashima/webgl-noise
 //               https://github.com/stegu/webgl-noise
 */
open GLSL;

let (mod289_2, mod289_3) = {
  let x2 = vec2arg("x");
  let x3 = vec3arg("x");
  let mod289_body = x =>
    body(
      {
        open! VertexShader;
        return(x - floor(x * (f(1.0) / f(289.0))) * f(289.0));
        finish();
      }
    );
  (
    fundecl(vec2fun("mod289"), [x2], mod289_body(x2)),
    fundecl(vec3fun("mod289"), [x3], mod289_body(x3))
  );
};

let permute = {
  let x = vec3arg("x");
  fundecl(
    vec3fun("permute"),
    [x],
    body(
      {
        open! VertexShader;
        return(mod289_3([(x * f(34.0) + f(1.0)) * x]));
        finish();
      }
    )
  );
};

let snoise = {
  let v = vec2arg("v");
  fundecl(
    vec3fun("snoise"),
    [v],
    body(
      {
        open! VertexShader;
        let cC = vec4var("C");
        cC
        =@ vec4([
             f(0.211324865405187), /* (3.0-sqrt(3.0))/6.0 */
             f(0.366025403784439), /* 0.5*(sqrt(3.0)-1.0) */
             f(-0.577350269189626), /* -1.0 + 2.0 * C.x */
             f(0.024390243902439)
           ]); /* 1.0 / 41.0 */
        /* First corner */
        let i = vec2var("i");
        i =@ floor(v + dot(v, cC **. YY));
        let x0 = vec2var("x0");
        x0 =@ v - i + dot(i, cC **. XX);
        /* Other corners */
        let i1 = vec2var("i1");
        i1
        =@ ternary(
             x0 **. X > x0 **. Y,
             vec2([f(1.0), f(0.0)]),
             vec2([f(0.0), f(1.0)])
           );
        let x12 = vec2var("x12");
        x12 =@ x0 **. XYXY + cC **. XXZZ;
        x12 **. XY -= i1;
        /* Permutations */
        i =@ mod289_2([i]); /* Avoid truncation effects in permutation */
        let p = vec3var("p");
        p
        =@ permute([
             permute([i **. Y + vec3([f(0.0), i1 **. Y, f(1.0)])])
             + i
             **. X
             + vec3([f(0.0), i1 **. X, f(1.0)])
           ]);
        let m = vec3var("m");
        m
        =@ max([
             f(0.5)
             - vec3([
                 dot(x0, x0),
                 dot(x12 **. XY, x12 **. XY),
                 dot(x12 **. ZW, x12 **. ZW)
               ]),
             f(0.0)
           ]);
        m =@ m * m;
        m =@ m * m;
        /* Gradients: 41 points uniformly over a line, mapped onto a diamond. */
        /* The ring size 17*17 = 289 is close to a multiple of 41 (41*7 = 287) */
        let x = vec3var("x");
        let h = vec3var("h");
        let ox = vec3var("ox");
        let a0 = vec3var("a0");
        x =@ f(2.0) * fract(p * (cC **. WWW)) - f(1.0);
        h =@ abs(x) - f(0.5);
        ox =@ floor(x + f(0.5));
        a0 =@ x - ox;
        /* Normalise gradients implicitly by scaling m */
        /* Approximation of: m *= inversesqrt( a0*a0 + h*h ); */
        m *= (f(1.79284291400159) - f(0.85373472095314) * (a0 * a0 + h * h));
        /* Compute final noise value at P */
        let g = vec3var("g");
        g **. X =@ a0 **. X * (x0 **. X) + h **. X * (x0 **. Y);
        g **. YZ =@ a0 **. YZ * (x12 **. XZ) + h **. YZ * (x12 **. YW);
        return(f(130.0) * dot(m, g));
        finish();
      }
    )
  );
};