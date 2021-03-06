// BEGIN LICENSE BLOCK
/*
Copyright � 2009 , UT-Battelle, LLC
All rights reserved

[DMRG++, Version 2.0.0]
[by G.A., Oak Ridge National Laboratory]

UT Battelle Open Source Software License 11242008

OPEN SOURCE LICENSE

Subject to the conditions of this License, each
contributor to this software hereby grants, free of
charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), a
perpetual, worldwide, non-exclusive, no-charge,
royalty-free, irrevocable copyright license to use, copy,
modify, merge, publish, distribute, and/or sublicense
copies of the Software.

1. Redistributions of Software must retain the above
copyright and license notices, this list of conditions,
and the following disclaimer.  Changes or modifications
to, or derivative works of, the Software should be noted
with comments and the contributor and organization's
name.

2. Neither the names of UT-Battelle, LLC or the
Department of Energy nor the names of the Software
contributors may be used to endorse or promote products
derived from this software without specific prior written
permission of UT-Battelle.

3. The software and the end-user documentation included
with the redistribution, with or without modification,
must include the following acknowledgment:

"This product includes software produced by UT-Battelle,
LLC under Contract No. DE-AC05-00OR22725  with the
Department of Energy."
 
*********************************************************
DISCLAIMER

THE SOFTWARE IS SUPPLIED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT OWNER, CONTRIBUTORS, UNITED STATES GOVERNMENT,
OR THE UNITED STATES DEPARTMENT OF ENERGY BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.

NEITHER THE UNITED STATES GOVERNMENT, NOR THE UNITED
STATES DEPARTMENT OF ENERGY, NOR THE COPYRIGHT OWNER, NOR
ANY OF THEIR EMPLOYEES, REPRESENTS THAT THE USE OF ANY
INFORMATION, DATA, APPARATUS, PRODUCT, OR PROCESS
DISCLOSED WOULD NOT INFRINGE PRIVATELY OWNED RIGHTS.

*********************************************************


*/
// END LICENSE BLOCK
#ifndef MODELHELPER_SU2_HEADER_H
#define MODELHELPER_SU2_HEADER_H

#include "BasisWithOperators.h"
#include "ClebschGordanCached.h"
#include "Su2Reduced.h"

/** \ingroup DMRG */
/*@{*/

/*! \file ModelHelperSu2.h
 *
 *  A class to contain state information about the Hamiltonian 
 *  to help with the calculation of x+=Hy (for when there's su2 symmetry)
 *
 */

namespace Dmrg { 	
	
	template<typename OperatorsType_,
		typename ReflectionSymmetryType_,
		typename ConcurrencyType_>
	class ModelHelperSu2  {
		typedef std::pair<size_t,size_t> PairType;
	public:	
		static const size_t System=0,Environ=1;
		typedef typename OperatorsType_::OperatorType OperatorType;
		typedef typename OperatorType::SparseMatrixType SparseMatrixType;
		typedef OperatorsType_ OperatorsType;
		typedef ReflectionSymmetryType_ ReflectionSymmetryType;
		typedef ConcurrencyType_ ConcurrencyType;
		typedef typename OperatorsType::BasisType BasisType;
		typedef typename BasisType::BlockType BlockType;
		typedef typename BasisType::RealType RealType;
		typedef BasisWithOperators<OperatorsType,ConcurrencyType> BasisWithOperatorsType;
		typedef typename SparseMatrixType::value_type SparseElementType;
		
		ModelHelperSu2(int m,const BasisType& basis1,  const BasisWithOperatorsType& basis2,
			 const BasisWithOperatorsType& basis3,size_t nOrbitals,bool useReflection=false) 
		:
			m_(m),
			basis1_(basis1),
			basis2_(basis2),
			basis3_(basis3),
			reflection_(useReflection),
			numberOfOperators_(basis2.numberOfOperatorsPerSite()),
			nOrbitals_(nOrbitals),	
			su2reduced_(m,basis1,basis2,basis3,nOrbitals)
		{}

		static bool isSu2() { return true; }

		const BasisType& basis1() const { return basis1_; }

		const BasisWithOperatorsType& basis2() const  { return basis2_; }

		const BasisWithOperatorsType& basis3() const  { return basis3_; }

		int size() const
		{
			int tmp = basis1_.partition(m_+1)-basis1_.partition(m_);
			return reflection_.size(tmp);
		}

		int quantumNumber() const
		{
			int state = basis1_.partition(m_);
			return basis1_.qn(state);
		}
	
		const SparseMatrixType& getReducedOperator(char modifier,size_t i,size_t sigma,size_t type) const
		{
			size_t i0 = i*numberOfOperators_ + sigma;
			if (type==System) return basis2_.getReducedOperatorByIndex(modifier,i0).data;
			return basis3_.getReducedOperatorByIndex(modifier,i0).data;
		}

		//! //! Does matrixBlock= (AB), A belongs to pSprime and B  belongs to pEprime or viceversa (inter)
		void fastOpProdInter(SparseMatrixType const &A,
				SparseMatrixType const &B,
				int type,
				SparseElementType  &hop,
				SparseMatrixType &matrixBlock,
				bool operatorsAreFermions,
    				size_t angularMomentum,
				RealType angularFactor,
    				size_t category,
				bool flip=false) const
		{
			int const SystemEnviron=1,EnvironSystem=2;
			RealType fermionSign =  (operatorsAreFermions) ? -1 : 1;

			if (type==EnvironSystem)  {
				SparseElementType hop2 = hop*fermionSign;
				fastOpProdInter(B,A,SystemEnviron,hop2,matrixBlock,operatorsAreFermions,
						angularMomentum,angularFactor,category,true);
				return;
			}

			//! work only on partition m
			int m = m_;
			int offset = basis1_.partition(m);
			int total = basis1_.partition(m+1) - offset;

			matrixBlock.resize(total);

			size_t counter=0;
			for (size_t i=0;i<su2reduced_.reducedEffectiveSize();i++) {
				int ix = su2reduced_.flavorMapping(i)-offset;
				if (ix<0 || ix>=int(matrixBlock.rank())) continue;
				matrixBlock.setRow(ix,counter);
				
				size_t i1=su2reduced_.reducedEffective(i).first;
				size_t i2=su2reduced_.reducedEffective(i).second;
				PairType jm1 = basis2_.jmValue(basis2_.reducedIndex(i1));
				
				size_t n1=basis2_.electrons(basis2_.reducedIndex(i1));
				RealType fsign=1;
				if (n1>0 && n1%2!=0) fsign= fermionSign;
				
				PairType jm2 = basis3_.jmValue(basis3_.reducedIndex(i2));
				size_t lf1 =jm1.first + jm2.first*basis2_.jMax();
					
				for (int k1=A.getRowPtr(i1);k1<A.getRowPtr(i1+1);k1++) {
					size_t i1prime = A.getCol(k1);
					PairType jm1prime = basis2_.jmValue(basis2_.reducedIndex(i1prime));

					for (int k2=B.getRowPtr(i2);k2<B.getRowPtr(i2+1);k2++) {
						size_t i2prime = B.getCol(k2);
						PairType jm2prime = basis3_.jmValue(basis3_.reducedIndex(i2prime));
						SparseElementType lfactor;
						size_t lf2 =jm1prime.first + jm2prime.first*basis2_.jMax();
						lfactor=su2reduced_.reducedFactor(angularMomentum,category,flip,lf1,lf2);
						if (lfactor==static_cast<SparseElementType>(0)) continue;

						lfactor *= angularFactor;

						int jx = su2reduced_.flavorMapping(i1prime,i2prime)-offset;
						if (jx<0 || jx >= int(matrixBlock.rank()) ) continue;

						matrixBlock.pushCol(jx);
						matrixBlock.pushValue(fsign*hop*lfactor*A.getValue(k1)*B.getValue(k2));
						counter++;
					}
				}
			}
			matrixBlock.setRow(matrixBlock.rank(),counter);
		}

		//! Does x+= (AB)y, where A belongs to pSprime and B  belongs to pEprime or viceversa (inter)
		//! Has been changed to accomodate for reflection symmetry
		 void fastOpProdInter(	std::vector<SparseElementType>  &x,
					std::vector<SparseElementType>  const &y,
					SparseMatrixType const &A,
					SparseMatrixType const &B,
					int type,
					SparseElementType  &hop,
					bool operatorsAreFermions,
     					size_t angularMomentum,
	  				RealType angularFactor,
       					size_t category,
	    				bool flip=false) const 
		{
			int const SystemEnviron=1,EnvironSystem=2;
			RealType fermionSign =  (operatorsAreFermions) ? -1 : 1;
			
			if (type==EnvironSystem)  {
				SparseElementType hop2 = hop*fermionSign;
				fastOpProdInter(x,y,B,A,SystemEnviron,hop2,operatorsAreFermions,
						angularMomentum,angularFactor,category,true);
				return;
			}

			//! work only on partition m
			int m = m_;
			int offset = basis1_.partition(m);

			for (size_t i=0;i<su2reduced_.reducedEffectiveSize();i++) {
				int ix = su2reduced_.flavorMapping(i)-offset;
				if (ix<0 || ix>=int(x.size())) continue;

				size_t i1=su2reduced_.reducedEffective(i).first;
				size_t i2=su2reduced_.reducedEffective(i).second;
				PairType jm1 = basis2_.jmValue(basis2_.reducedIndex(i1));
				size_t n1=basis2_.electrons(basis2_.reducedIndex(i1));
				RealType fsign=1;

				if (n1>0 && n1%2!=0) fsign= fermionSign;

				PairType jm2 = basis3_.jmValue(basis3_.reducedIndex(i2));
				size_t lf1 =jm1.first + jm2.first*basis2_.jMax();

				for (int k1=A.getRowPtr(i1);k1<A.getRowPtr(i1+1);k1++) {
					size_t i1prime = A.getCol(k1);
					PairType jm1prime = basis2_.jmValue(basis2_.reducedIndex(i1prime));

					for (int k2=B.getRowPtr(i2);k2<B.getRowPtr(i2+1);k2++) {
						size_t i2prime = B.getCol(k2);
						PairType jm2prime = basis3_.jmValue(basis3_.reducedIndex(i2prime));
						SparseElementType lfactor;
						size_t lf2 =jm1prime.first + jm2prime.first*basis2_.jMax();

						lfactor=su2reduced_.reducedFactor(angularMomentum,category,flip,lf1,lf2);
						if (lfactor==static_cast<SparseElementType>(0)) continue;
						lfactor *= angularFactor;

						int jx = su2reduced_.flavorMapping(i1prime,i2prime)-offset;
						if (jx<0 || jx >= int(y.size()) ) continue;

						x[ix] += fsign*hop*lfactor*A.getValue(k1)*B.getValue(k2)*y[jx];
					}
				}
			}
		}

		//! Let H_{alpha,beta; alpha',beta'} = basis2.hamiltonian_{alpha,alpha'} \delta_{beta,beta'}
		//! Let H_m be  the m-th block (in the ordering of basis1) of H
		//! Then, this function does x += H_m * y
		//! This is a performance critical function
		//! Has been changed to accomodate for reflection symmetry
		void hamiltonianLeftProduct(std::vector<SparseElementType> &x,std::vector<SparseElementType> const &y) const 
		{ 
			//! work only on partition m
			int m = m_;
			int offset = basis1_.partition(m);
			const SparseMatrixType& A = su2reduced_.hamiltonianLeft();

			for (size_t i=0;i<su2reduced_.reducedEffectiveSize();i++) {
				int ix = su2reduced_.flavorMapping(i)-offset;
				if (ix<0 || ix>=int(x.size())) continue;

				size_t i1=su2reduced_.reducedEffective(i).first;
				size_t i2=su2reduced_.reducedEffective(i).second;

				PairType jm1 = basis2_.jmValue(basis2_.reducedIndex(i1));
				PairType jm2 = basis3_.jmValue(basis3_.reducedIndex(i2));

				for (int k1=A.getRowPtr(i1);k1<A.getRowPtr(i1+1);k1++) {
					size_t i1prime = A.getCol(k1);
					SparseElementType lfactor=su2reduced_.reducedHamiltonianFactor(jm1.first,jm2.first);

					if (lfactor==static_cast<SparseElementType>(0)) continue;

					int jx = su2reduced_.flavorMapping(i1prime,i2)-offset;
					if (jx<0 || jx >= int(y.size()) ) continue;

					x[ix] += A.getValue(k1)*y[jx];
				}
			}
		}

		//! Let  H_{alpha,beta; alpha',beta'} = basis2.hamiltonian_{beta,beta'} \delta_{alpha,alpha'}
		//! Let H_m be  the m-th block (in the ordering of basis1) of H
		//! Then, this function does x += H_m * y
		//! This is a performance critical function
		void hamiltonianRightProduct(std::vector<SparseElementType> &x,std::vector<SparseElementType> const &y) const 
		{ 
			//! work only on partition m
			int m = m_;
			int offset = basis1_.partition(m);
			const SparseMatrixType& B = su2reduced_.hamiltonianRight();

			for (size_t i=0;i<su2reduced_.reducedEffectiveSize();i++) {
				int ix = su2reduced_.flavorMapping(i)-offset;
				if (ix<0 || ix>=int(x.size())) continue;

				size_t i1=su2reduced_.reducedEffective(i).first;
				size_t i2=su2reduced_.reducedEffective(i).second;
				PairType jm1 = basis2_.jmValue(basis2_.reducedIndex(i1));
				PairType jm2 = basis3_.jmValue(basis3_.reducedIndex(i2));

				for (int k2=B.getRowPtr(i2);k2<B.getRowPtr(i2+1);k2++) {
					size_t i2prime = B.getCol(k2);
					SparseElementType lfactor=su2reduced_.reducedHamiltonianFactor(jm1.first,jm2.first);
					if (lfactor==static_cast<SparseElementType>(0)) continue;

					int jx = su2reduced_.flavorMapping(i1,i2prime)-offset;
					if (jx<0 || jx >= int(y.size()) ) continue;

					x[ix] += B.getValue(k2)*y[jx];
				}
			}
		}

		//! Note: USed only for debugging
		void calcHamiltonianPartLeft(SparseMatrixType &matrixBlock) const
		{
			//! work only on partition m
			int m = m_;
			int offset = basis1_.partition(m);
			int bs = basis1_.partition(m+1)-offset;
			const SparseMatrixType& A = su2reduced_.hamiltonianLeft();
			
			matrixBlock.resize(bs);
			size_t counter=0;
			for (size_t i=0;i<su2reduced_.reducedEffectiveSize();i++) {
				int ix = su2reduced_.flavorMapping(i)-offset;
				matrixBlock.setRow(ix,counter);
				if (ix<0 || ix>=int(matrixBlock.rank())) continue;

				size_t i1=su2reduced_.reducedEffective(i).first;
				size_t i2=su2reduced_.reducedEffective(i).second;
				PairType jm1 = basis2_.jmValue(basis2_.reducedIndex(i1));
				PairType jm2 = basis3_.jmValue(basis3_.reducedIndex(i2));

				for (int k1=A.getRowPtr(i1);k1<A.getRowPtr(i1+1);k1++) {
					size_t i1prime = A.getCol(k1);
					SparseElementType lfactor=su2reduced_.reducedHamiltonianFactor(jm1.first,jm2.first);

					if (lfactor==static_cast<SparseElementType>(0)) continue;
						
					int jx = su2reduced_.flavorMapping(i1prime,i2)-offset;
					if (jx<0 || jx >= int(matrixBlock.rank()) ) continue;

					matrixBlock.pushCol(jx);
					matrixBlock.pushValue( A.getValue(k1));
					counter++;
				}
			}
			matrixBlock.setRow(bs,counter);
		}

		//! Note: USed only for debugging
		void calcHamiltonianPartRight(SparseMatrixType &matrixBlock) const
		{
			//! work only on partition m
			int m = m_;
			int offset = basis1_.partition(m);
			int bs = basis1_.partition(m+1)-offset;
			const SparseMatrixType& B = su2reduced_.hamiltonianRight();

			matrixBlock.resize(bs);
			size_t counter=0;
			for (size_t i=0;i<su2reduced_.reducedEffectiveSize();i++) {
				int ix = su2reduced_.flavorMapping(i)-offset;
				matrixBlock.setRow(ix,counter);
				if (ix<0 || ix>=int(matrixBlock.rank())) continue;

				size_t i1=su2reduced_.reducedEffective(i).first;
				size_t i2=su2reduced_.reducedEffective(i).second;
				PairType jm1 = basis2_.jmValue(basis2_.reducedIndex(i1));
				PairType jm2 = basis3_.jmValue(basis3_.reducedIndex(i2));

				for (int k2=B.getRowPtr(i2);k2<B.getRowPtr(i2+1);k2++) {
					size_t i2prime = B.getCol(k2);
					SparseElementType lfactor=su2reduced_.reducedHamiltonianFactor(jm1.first,jm2.first);
					if (lfactor==static_cast<SparseElementType>(0)) continue;

					int jx = su2reduced_.flavorMapping(i1,i2prime)-offset;
					if (jx<0 || jx >= int(matrixBlock.rank()) ) continue;

					matrixBlock.pushCol(jx);
					matrixBlock.pushValue(B.getValue(k2));
					counter++;
				}
			}
			matrixBlock.setRow(bs,counter);
		}

		//! if option==true let H_{alpha,beta; alpha',beta'} = basis2.hamiltonian_{alpha,alpha'} \delta_{beta,beta'}
		//! if option==false let  H_{alpha,beta; alpha',beta'} = basis2.hamiltonian_{beta,beta'} \delta_{alpha,alpha'}
		//! returns the m-th block (in the ordering of basis1) of H
		//! Note: USed only for debugging
		void calcHamiltonianPart(SparseMatrixType &matrixBlock,bool option) const 
		{ 
			if (option) calcHamiltonianPartLeft(matrixBlock);
			else calcHamiltonianPartRight(matrixBlock);
		}

		void getReflectedEigs(
				RealType& energyTmp,std::vector<SparseElementType>& tmpVec,
				RealType energyTmp1,const std::vector<SparseElementType>& tmpVec1,
				RealType energyTmp2,const std::vector<SparseElementType>& tmpVec2) const
		{
			reflection_.getReflectedEigs(energyTmp,tmpVec,energyTmp1,tmpVec1,energyTmp2,tmpVec2);
		}

		void setReflectionSymmetry(size_t reflectionSector)
		{
			reflection_.setReflectionSymmetry(reflectionSector);
		}

		void printFullMatrix(const SparseMatrixType& matrix) const
                {
                        reflection_.printFullMatrix(matrix);
                }

		void printFullMatrixMathematica(const SparseMatrixType& matrix) const
                {
                        reflection_.printFullMatrixMathematica(matrix);
                }	

		size_t m() const {return m_;}

	private:
		int m_;
		const BasisType&  basis1_;
		const BasisWithOperatorsType& basis2_;
		const BasisWithOperatorsType& basis3_;
		ReflectionSymmetryType reflection_;
		size_t numberOfOperators_,nOrbitals_;
		Su2Reduced<OperatorsType_,ReflectionSymmetryType_,ConcurrencyType_>
				su2reduced_;
	};
} // namespace Dmrg
/*@}*/

#endif

