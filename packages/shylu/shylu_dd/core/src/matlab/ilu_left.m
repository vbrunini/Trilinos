function [L,U,P] = ilu_left (A, setup)
%ILU_LEFT left-looking ILU factorization.
% Example:
%   [L,U,P] = ilu_left (A, setup)
% See also: cs_demo

%   Copyright 2006-2007, Timothy A. Davis.
%   http://www.cise.ufl.edu/research/sparse
%
%   Modified by Erik Boman for ILU.
%   The setup argument is compatible with Matlab's ilu.

if (nargin<2)
  setup.droptol = 0;                    % default is complete LU
end

n = size (A,1) ;
P = eye (n) ;
L = zeros (n) ;
U = zeros (n) ;
for k = 1:n
    x = [ L(:,1:k-1) [ zeros(k-1,n-k+1) ; eye(n-k+1) ]] \ (P * A (:,k)) ;
    keep = (abs(x)/norm(A(:,k)) > setup.droptol);  
    keep(k) = 1;                        % always keep diagonal
    x = x .* keep;                      % drop small entries in x
    U (1:k-1,k) = x (1:k-1) ;           % the column of U
    [a i] = max (abs (x (k:n))) ;       % find the pivot row i
    i = i + k - 1 ;
    L ([i k],:) = L ([k i], :) ;        % swap rows i and k of L, P, and x
    P ([i k],:) = P ([k i], :) ;
    x ([i k]) = x ([k i]) ;
    U (k,k) = x (k) ;
    L (k,k) = 1 ;
    L (k+1:n,k) = x (k+1:n) / x (k) ;   % divide the pivot column by U(k,k)
end
