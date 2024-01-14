{ lib
, llvmPackages_17
, cmake
}:

llvmPackages_17.libcxxStdenv.mkDerivation rec {
  pname = "wikidice";
  version = "2.0.0";
  
  src = ./.;

  nativeBuildInputs = [ cmake ninja ];
  buildInputs = [
    zstd
  ];

  cmakeFlags = [
    "-DENABLE_TESTING=OFF"
    "-DENABLE_INSTALL=ON"
  ];
}
