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
#ifndef SU2_REDUCED_HEADER_H
#define SU2_REDUCED_HEADER_H

#include "BasisWithOperators.h"
#include "Su2SymmetryGlobals.h"

/** \ingroup DMRG */
/*@{*/

/*! \file Su2Reduced.h
 *
 *  Using WignerEckart Theorem to speed up SU(2) algorithm
 *
 */
namespace Dmrg {
	template<typename OperatorsType,
		typename ReflectionSymmetryType,
		typename ConcurrencyType>
	class Su2Reduced {
			typedef typename OperatorsType::SparseMatrixType SparseMatrixType;
			typedef typename SparseMatrixType::value_type SparseElementType;
			typedef BasisWithOperators<OperatorsType,ConcurrencyType> BasisWithOperatorsType;
			typedef typename OperatorsType::BasisType BasisType;
			typedef typename BasisType::RealType RealType;
			typedef std::pair<size_t,size_t> PairType;
			typedef typename OperatorsType::OperatorType OperatorType;
			typedef Su2SymmetryGlobals<RealType> Su2SymmetryGlobalsType;
			typedef typename Su2SymmetryGlobalsType::ClebschGordanType ClebschGordanType;
			
			static const size_t System=0,Environ=1;
			static const BasisType* basis1Ptr_;

			size_t m_;
			BasisType basis1_;
			BasisWithOperatorsType basis2_,basis3_;
			size_t nOrbitals_;
			std::vector<psimag::Matrix<SparseElementType> > lfactor_;
			psimag::Matrix<SparseElementType> lfactorHamiltonian_;
			SparseMatrixType hamiltonian2_,hamiltonian3_;
			std::vector<PairType> reducedEffective_;
			psimag::Matrix<size_t> reducedInverse_;
			std::vector<size_t> flavorsOldInverse_;
			ClebschGordanType& cgObject_;

		public:
			Su2Reduced(int m,const BasisType& basis1,  const BasisWithOperatorsType& basis2,
			 const BasisWithOperatorsType& basis3,size_t nOrbitals,bool useReflection=false) :
			m_(m),
			basis1_(basis1),
			basis2_(basis2),
			basis3_(basis3),
			nOrbitals_(nOrbitals),
			cgObject_(Su2SymmetryGlobalsType::clebschGordanObject)
			{
				std::vector<PairType> jsEffective;
				std::vector<size_t> jvalues;
				// find all possible j values
				size_t counter=0;
				for (size_t i=0;i<basis2.numberOfOperators();i++) {
					size_t j = basis2.getReducedOperatorByIndex(i).jm.first;
					int x = utils::isInVector(jvalues,j);
					if (x<0) jvalues.push_back(j);
					counter += (j+1);
				}

				// build all lfactors
				lfactor_.resize(counter);
				counter=0;
				for (size_t i=0;i<jvalues.size();i++) {
					for (size_t m=0;m<=jvalues[i];m++) { 
						buildAdditional(lfactor_[counter],jvalues[i],m,jvalues[i]-m,jsEffective);
						counter++;
					}
				}

				calcHamFactor(lfactorHamiltonian_);
				calcEffectiveStates(jsEffective);

				basis1Ptr_ = &basis1;
				createReducedHamiltonian(hamiltonian2_,basis2_);

				createReducedHamiltonian(hamiltonian3_,basis3_);
			}

			const PairType& reducedEffective(size_t i) const
			{
				return reducedEffective_[i];
			}

			size_t reducedEffectiveSize() const
			{
				return 	reducedEffective_.size();
			}

			size_t flavorMapping(size_t i1prime,size_t i2prime) const
			{
				return flavorsOldInverse_[reducedInverse_(i1prime,i2prime)];
			}

			size_t flavorMapping(size_t i) const
			{
				return flavorsOldInverse_[i];
			}

			SparseElementType reducedFactor(size_t angularMomentum,size_t category,bool flip,size_t lf1,size_t lf2) const
			{
				size_t category2 = category;
				if (flip) category2 = angularMomentum-category;
				SparseElementType lfactor=lfactor_[category2](lf1,lf2);
				return lfactor;
			}

			SparseElementType reducedHamiltonianFactor(size_t j1,size_t j2) const
			{
				return lfactorHamiltonian_(j1,j2);
			}

			const SparseMatrixType& hamiltonianLeft() const 
			{
				return hamiltonian2_;	
			}

			const SparseMatrixType& hamiltonianRight() const 
			{
				return hamiltonian3_;	
			}

		private:
			void createReducedHamiltonian(SparseMatrixType& hamReduced,const BasisWithOperatorsType& basis)
			{
				hamReduced.resize(basis.numberOfOperators());
				std::vector<size_t> basisrinverse(basis.size());
				for (size_t i=0;i<basis.size();i++) {
					size_t f = basis.getFlavor(i);
					size_t j = basis.jmValue(i).first;
					basisrinverse[i]=findJf(basis,j,f);
				}

				size_t angularMomentum =0;
				std::vector<const OperatorType*> opSrc(angularMomentum+1);

				OperatorType myOp; 
				myOp.data = basis.hamiltonian();
				myOp.fermionSign=1;
				myOp.jm=typename OperatorType::PairType(0,0);	
				myOp.angularFactor = 1.0;
				opSrc[0]=&myOp;
				createReducedOperator(hamReduced,opSrc,basis,basisrinverse,basis.reducedSize(),0);
			}

			size_t findJf(const BasisWithOperatorsType& basis,size_t j,size_t f)
			{
				for (size_t i=0;i<basis.reducedSize();i++) {
					PairType jm=basis.jmValue(basis.reducedIndex(i));
					if (jm.first!=j) continue;
					if (basis.getFlavor(basis.reducedIndex(i))!=f) continue;
					return i;	
				}
				return 0; // bogus
			}

			void createReducedConj(size_t k1 ,SparseMatrixType& opDest,const SparseMatrixType& opSrc,
					       const BasisWithOperatorsType& basis,size_t counter)
			{
				size_t n=opSrc.rank();
				opDest=transposeConjugate(opSrc);
				for (size_t i=0;i<n;i++) {
					PairType jm = basis.jmValue(basis.reducedIndex(i));
					for (int k=opDest.getRowPtr(i);k<opDest.getRowPtr(i+1);k++) {
						size_t j=opDest.getCol(k);
						PairType jmPrime = basis.jmValue(basis.reducedIndex(j));
						
						SparseElementType factor=SparseElementType(jm.first+1)/SparseElementType(jmPrime.first+1);
						factor = sqrt(factor);
						int x = k1+jm.first-jmPrime.first;
						x = int(x/2);
						if (x%2!=0) factor = -1.0*factor;
						SparseElementType val=opDest.getValue(k)*factor;
						opDest.setValues(k,val);
					}
				}
			}

			void createReducedOperator(
					SparseMatrixType& opDest,
					const std::vector<const OperatorType*>& opSrc,
					const BasisWithOperatorsType& basis,
     					const std::vector<size_t>& basisrInverse,
	  				size_t n,
       					size_t counter)
			{
				psimag::Matrix<SparseElementType> opDest1(n,n);
				for (size_t i=0;i<opSrc.size();i++) 
					createReducedOperator(opDest1,*opSrc[i],basis,basisrInverse); //,PairType(k,mu1),mysign);
				fullMatrixToCrsMatrix(opDest,opDest1);
			}

			void createReducedOperator(
					psimag::Matrix<SparseElementType>& opDest1,
					const OperatorType& opSrc,
     					const BasisWithOperatorsType& basis,
     					const std::vector<size_t>& basisrInverse)
			{
				for (int i=0;i<opSrc.data.rank();i++) {
					PairType jm = basis.jmValue(i);
					for (int l=opSrc.data.getRowPtr(i);l<opSrc.data.getRowPtr(i+1);l++) {
						size_t iprime = opSrc.data.getCol(l);
						PairType jmPrime = basis.jmValue(iprime);

						RealType divisor = opSrc.angularFactor*(jmPrime.first+1);
						opDest1(basisrInverse[i],basisrInverse[iprime]) += 
									opSrc.data(i,iprime)*cgObject_(jmPrime,jm,opSrc.jm)/divisor;
						
					}
				}
			}

			void buildAdditional(psimag::Matrix<SparseElementType>& lfactor,size_t k,size_t mu1,size_t mu2,
					     std::vector<PairType>& jsEffective)
			{
				PairType kmu1(k,mu1);
				PairType kmu2(k,mu2);
				size_t counter=0;	
				int offset = basis1_.partition(m_);
				PairType jm=basis1_.jmValue(offset);
				lfactor.resize(basis2_.jMax()*basis3_.jMax(),basis2_.jMax()*basis3_.jMax());			
				for (size_t i1=0;i1<basis2_.jVals();i1++) {
					for (size_t i2=0;i2<basis3_.jVals();i2++) {
						for (size_t i1prime=0;i1prime<basis2_.jVals();i1prime++) {
							for (size_t i2prime=0;i2prime<basis3_.jVals();i2prime++) {
								SparseElementType sum=calcLfactor(basis2_.jVals(i1),basis3_.jVals(i2),
										basis2_.jVals(i1prime),basis3_.jVals(i2prime),jm,kmu1,kmu2);
								if (sum!=static_cast<SparseElementType>(0)) {
									counter++;
									PairType jj(PairType(basis2_.jVals(i1),basis3_.jVals(i2)));
									int x3=utils::isInVector(jsEffective,jj);
									if (x3<0) jsEffective.push_back(jj);
								}
								lfactor(basis2_.jVals(i1)+basis3_.jVals(i2)*basis2_.jMax(),
									basis2_.jVals(i1prime)+basis3_.jVals(i2prime)*basis2_.jMax())=sum;
								
							}
						}
					}
				}
			}

			void calcHamFactor(psimag::Matrix<SparseElementType>& lfactor)
			{
				int offset = basis1_.partition(m_);
				PairType jm=basis1_.jmValue(offset);
				lfactor.resize(basis2_.jMax(),basis3_.jMax());
				PairType kmu(0,0);
				for (size_t i1=0;i1<basis2_.jVals();i1++) {
					for (size_t i2=0;i2<basis3_.jVals();i2++) {
						lfactor(basis2_.jVals(i1),basis3_.jVals(i2))=
							calcLfactor(basis2_.jVals(i1),basis3_.jVals(i2),
									basis2_.jVals(i1),basis3_.jVals(i2),jm,kmu,kmu);
					}
				}
			}

			void calcEffectiveStates(std::vector<PairType> jsEffective)
			{
				int offset = basis1_.partition(m_);
				PairType jm=basis1_.jmValue(offset);
				reducedEffective_.clear();
				reducedInverse_.resize(basis2_.reducedSize(),basis3_.reducedSize());
				size_t electrons = basis1_.electrons(offset);
				for (size_t i=0;i<jsEffective.size();i++) {
					for (size_t i1=0;i1<basis2_.reducedSize();i1++) {
						for (size_t i2=0;i2<basis3_.reducedSize();i2++) {
							if (electrons!=basis2_.electrons(basis2_.reducedIndex(i1))+
									basis3_.electrons(basis3_.reducedIndex(i2))) continue;
							PairType jj(basis2_.jmValue(basis2_.reducedIndex(i1)).first,
									basis3_.jmValue(basis3_.reducedIndex(i2)).first);
							if (jj!=jsEffective[i]) continue;
							reducedEffective_.push_back(PairType(i1,i2));
							reducedInverse_(i1,i2)=reducedEffective_.size()-1;
						}
					}
				}

				getFlavorMap(jm);

				reorderMap();		
			}

			SparseElementType calcLfactor(size_t j1,size_t j2,size_t j1prime,size_t j2prime,const PairType& jm,
						const PairType& kmu1,const PairType& kmu2) const
			{
				SparseElementType sum=0;
				size_t k = kmu1.first;
				size_t mu1=kmu1.second;
				size_t mu2=kmu2.second;

				for (size_t m1=0;m1<=j1;m1++) {
					PairType jm1(j1,m1);
					int x = j1+j2-jm.first;
					if (x%2!=0) continue;
					if (int(int(x/2)-m1 + jm.second)<0) continue;
					size_t m2 = size_t(x/2)-m1 + jm.second;
					PairType jm2(j2,m2);
					if (m2>j2) continue;

					x = j1prime-k-j1;
					if (x%2!=0) continue;
					if (m1+mu1+int(x/2)<0) continue;
					size_t m1prime = m1+mu1+size_t(x/2);
					if (m1prime>j1prime) continue;
					PairType jm1prime(j1prime,m1prime); 

					x = j2prime-k-j2;
					if (x%2!=0) continue;
					if (m2+mu2+int(x/2)<0) continue;
					size_t m2prime = m2+mu2+size_t(x/2);
					if (m2prime>j2prime) continue;
					PairType jm2prime(j2prime,m2prime); 

					x=j1prime+j2prime-jm.first;
					if (x%2!=0) continue;
					x= x/2;
					x += jm.second -m1prime-m2prime;
					if (x!=0) continue;

					sum +=  cgObject_(jm1prime,jm1,kmu1)*
						cgObject_(jm2prime,jm2,kmu2)*
						cgObject_(jm,jm1,jm2)*cgObject_(jm,jm1prime,jm2prime);
						
				}
				return sum;
			}

			void getFlavorMap(const PairType& jm)
			{
				std::map<size_t,size_t> flavorsOldInverse;
				basis1_.flavor2Index(flavorsOldInverse,jm);
				flavorsOldInverse_.resize(reducedEffective_.size());

				for (size_t i=0;i<reducedEffective_.size();i++) {
					size_t i1= basis2_.reducedIndex(reducedEffective_[i].first);
					size_t f1= basis2_.getFlavor(i1);
					size_t ne1= basis2_.electrons(i1);
					size_t j1= basis2_.jmValue(i1).first;

					size_t i2= basis3_.reducedIndex(reducedEffective_[i].second);
					size_t f2= basis3_.getFlavor(i2);
					size_t ne2= basis3_.electrons(i2);
					size_t j2= basis3_.jmValue(i2).first;

					size_t f=basis1_.flavor2Index(f1,f2,ne1,ne2,j1,j2);
					flavorsOldInverse_[i]=flavorsOldInverse[f];
				}
			}

			void reorderMap()
			{
				if (flavorsOldInverse_.size()==0) return;
				std::vector<size_t> perm(flavorsOldInverse_.size());
				utils::sort(flavorsOldInverse_,perm);
				std::vector<PairType> r(reducedEffective_.size());
				psimag::Matrix<size_t> reducedInverse(reducedInverse_.n_row(),reducedInverse_.n_col());

				for (size_t i=0;i<reducedEffective_.size();i++) {
					r[i]=reducedEffective_[perm[i]];
					reducedInverse(r[i].first,r[i].second)=i;
				}
				reducedEffective_=r;
				reducedInverse_=reducedInverse;
			}

	}; // class

	template<typename OperatorsType,
		typename ReflectionSymmetryType,
		typename ConcurrencyType>
	const typename OperatorsType::BasisType* Su2Reduced<
 	OperatorsType,ReflectionSymmetryType,ConcurrencyType>::basis1Ptr_=0;
} // namespace Dmrg
/*@}*/
#endif
