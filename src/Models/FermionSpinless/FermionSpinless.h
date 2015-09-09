/*
Copyright (c) 2009-2015, UT-Battelle, LLC
All rights reserved

[DMRG++, Version 3.0]
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

/** \ingroup DMRG */
/*@{*/

/*! \file FermionSpinless.h
 *
 *  TBW
 *
 */
#ifndef DMRG_FERMION_SPINLESS
#define DMRG_FERMION_SPINLESS
#include <cassert>
#include "Sort.h" // in PsimagLite
#include "ParametersFermionSpinless.h"
#include "../HubbardOneBand/HilbertSpaceHubbard.h"
#include "LinkProductHubbardOneBand.h"
#include "CrsMatrix.h"
#include "SpinSquaredHelper.h"
#include "SpinSquared.h"
#include "VerySparseMatrix.h"
#include "ProgramGlobals.h"
#include "ModelCommon.h"

namespace Dmrg {
//! Model Hubbard for DMRG solver, inherits from ModelBase and implements its interface:
template<typename ModelBaseType>
class FermionSpinless : public ModelBaseType {

public:

	typedef typename ModelBaseType::ModelHelperType ModelHelperType;
	typedef typename ModelBaseType::GeometryType GeometryType;
	typedef typename ModelBaseType::LeftRightSuperType LeftRightSuperType;
	typedef typename ModelBaseType::LinkProductStructType LinkProductStructType;
	typedef typename ModelBaseType::LinkType LinkType;
	typedef typename ModelHelperType::OperatorsType OperatorsType;
	typedef typename OperatorsType::OperatorType OperatorType;
	typedef typename ModelHelperType::RealType RealType;
	typedef TargetQuantumElectrons<RealType> TargetQuantumElectronsType;
	typedef typename ModelBaseType::SparseMatrixType SparseMatrixType;
	typedef typename ModelHelperType::SparseElementType SparseElementType;
	typedef unsigned int long long WordType;
	typedef  HilbertSpaceHubbard<WordType> HilbertSpaceHubbardType;
	typedef typename ModelBaseType::VectorOperatorType VectorOperatorType;
	typedef typename ModelBaseType::SymmetryElectronsSzType SymmetryElectronsSzType;
	typedef typename ModelBaseType::VectorSizeType VectorSizeType;

private:

	static const int FERMION_SIGN = -1;
	static const int DEGREES_OF_FREEDOM=1;
	static const int NUMBER_OF_ORBITALS=1;

	enum {SPIN_UP = HilbertSpaceHubbardType::SPIN_UP,
		  SPIN_DOWN = HilbertSpaceHubbardType::SPIN_DOWN};

	typedef typename ModelBaseType::BlockType BlockType;
	typedef typename ModelBaseType::SolverParamsType SolverParamsType;
	typedef typename ModelBaseType::VectorType VectorType;

public:

	typedef typename HilbertSpaceHubbardType::HilbertState HilbertState;
	typedef LinkProductHubbardOneBand<ModelHelperType> LinkProductType;
	typedef ModelCommon<ModelBaseType,LinkProductType> ModelCommonType;
	typedef typename ModelBaseType::InputValidatorType InputValidatorType;
	typedef	typename ModelBaseType::MyBasis MyBasis;
	typedef	typename ModelBaseType::BasisWithOperatorsType MyBasisWithOperators;
	typedef typename PsimagLite::Vector<HilbertState>::Type HilbertBasisType;

	FermionSpinless(const SolverParamsType& solverParams,
	                InputValidatorType& io,
	                GeometryType const &geometry,
	                SizeType offset = DEGREES_OF_FREEDOM)
	    : ModelBaseType(io,new ModelCommonType(solverParams,geometry)),
	      modelParameters_(io),
	      geometry_(geometry),
	      offset_(offset),
	      spinSquared_(spinSquaredHelper_,NUMBER_OF_ORBITALS,DEGREES_OF_FREEDOM)
	{}

	SizeType memResolv(PsimagLite::MemResolv&,
	                   SizeType,
	                   PsimagLite::String = "") const
	{
		return 0;
	}

	/** returns the size of the one-site Hilbert space. */
	SizeType hilbertSize(SizeType) const
	{
		return (SizeType)pow(2,NUMBER_OF_ORBITALS);
	}

	/** Function \cppFunction{!PTEX_THISFUNCTION} sets certain aspects of the
		``natural basis'' (usually the real-space basis) on a given block.
		The operator matrices (e.g., $c^\dagger_{i\sigma}$ for the Hubbard
		model or $S_i^+$ and $S_i^z$ for the Heisenberg model) need to be set
		there, as well as the Hamiltonian and the effective quantum number for
		each state of this natural basis. To implement the algorithm for a
		fixed density, the number of electrons for each state is also needed.*/
	virtual void setNaturalBasis(VectorOperatorType& creationMatrix,
	                             SparseMatrixType &hamiltonian,
	                             SymmetryElectronsSzType& q,
	                             const BlockType& block,
	                             const RealType& time) const
	{
		HilbertBasisType natBasis;
		typename PsimagLite::Vector<SizeType>::Type quantumNumbs;
		setNaturalBasis(natBasis,quantumNumbs,block);

		setOperatorMatrices(creationMatrix,block);

		//! Set symmetry related
		setSymmetryRelated(q,natBasis,block.size());

		//! set hamiltonian
		this->calcHamiltonian(hamiltonian,creationMatrix,block,time);
	}

	/** \cppFunction{!PTEX_THISFUNCTION} sets local operators needed to
		 construct the Hamiltonian.
		 For example, for the Hubbard model these operators are the
		 creation operators for sites in block */
	virtual void setOperatorMatrices(VectorOperatorType& creationMatrix,
	                                 const BlockType& block) const
	{
		HilbertBasisType natBasis;
		SparseMatrixType tmpMatrix;
		typename PsimagLite::Vector<SizeType>::Type quantumNumbs;
		setNaturalBasis(natBasis,quantumNumbs,block);

		//! Set the operators c^\daggger_{i\sigma} in the natural basis
		creationMatrix.clear();
		for (SizeType i=0;i<block.size();i++) {
			for (int sigma=0;sigma<DEGREES_OF_FREEDOM;sigma++) {
				tmpMatrix = findOperatorMatrices(i,sigma,natBasis);
				int asign= 1;
				if (sigma>0) asign= 1;
				typename OperatorType::Su2RelatedType su2related;
				if (sigma==0) {
					su2related.source.push_back(i*offset_);
					su2related.source.push_back(i*offset_+1);
					su2related.transpose.push_back(-1);
					su2related.transpose.push_back(-1);
					su2related.offset = NUMBER_OF_ORBITALS;
				}
				OperatorType myOp(tmpMatrix,
				                  -1,
				                  typename OperatorType::PairType(1,1-sigma),
				                  asign,
				                  su2related);

				creationMatrix.push_back(myOp);
			}
		}
	}

	/** \cppFunction{!PTEX_THISFUNCTION} returns the operator in the
	 * unmangled (natural) basis of one-site */
	PsimagLite::Matrix<SparseElementType> naturalOperator(const PsimagLite::String& what,
	                                                      SizeType site,
	                                                      SizeType dof) const
	{
		BlockType block;
		block.resize(1);
		block[0]=site;
		typename PsimagLite::Vector<OperatorType>::Type creationMatrix;
		setOperatorMatrices(creationMatrix,block);
		assert(creationMatrix.size()>0);
		SizeType nrow = creationMatrix[0].data.row();

		if (what == "i" || what=="identity") {
			PsimagLite::Matrix<SparseElementType> tmp(nrow,nrow);
			for (SizeType i = 0; i < tmp.n_row(); ++i) tmp(i,i) = 1.0;
			return tmp;
		}

		if (what=="n") {
			PsimagLite::Matrix<SparseElementType> tmp =
			        multiplyTc(creationMatrix[0].data,creationMatrix[0].data);
			return tmp;
		}

		if (what=="c") {
			PsimagLite::Matrix<SparseElementType> tmp;
			assert(dof<creationMatrix.size());
			crsMatrixToFullMatrix(tmp,creationMatrix[dof].data);
			return tmp;
		}

		std::cerr<<"Argument: "<<what<<" "<<__FILE__<<"\n";
		throw std::logic_error("naturalOperator(): invalid argument\n");
	}

	/** \cppFunction{!PTEX_THISFUNCTION} Sets electrons to the total number of
		electrons for each state in the basis*/
	void findElectrons(typename PsimagLite::Vector<SizeType> ::Type&electrons,
	                   const typename PsimagLite::Vector<HilbertState>::Type& basis,
	                   SizeType) const
	{
		electrons.clear();
		for (SizeType i=0;i<basis.size();i++) {
			int nup = HilbertSpaceHubbardType::getNofDigits(basis[i],0);
			electrons.push_back(nup);
		}
	}

	void setNaturalBasis(HilbertBasisType  &basis,
	                     typename PsimagLite::Vector<SizeType>::Type& q,
	                     const typename PsimagLite::Vector<SizeType>::Type& block) const
	{
		HilbertState a=0;
		int sitesTimesDof=DEGREES_OF_FREEDOM*block.size();
		HilbertState total = (1<<sitesTimesDof);

		HilbertBasisType  basisTmp;
		for (a=0;a<total;a++) basisTmp.push_back(a);

		// reorder the natural basis (needed for MULTIPLE BANDS)
		findQuantumNumbers(q,basisTmp,1);
		typename PsimagLite::Vector<SizeType>::Type iperm(q.size());

		PsimagLite::Sort<typename PsimagLite::Vector<SizeType>::Type > sort;
		sort.sort(q,iperm);
		basis.clear();
		for (a=0;a<total;a++) basis.push_back(basisTmp[iperm[a]]);
	}

	void print(std::ostream& os) const
	{
		os<<modelParameters_;
	}

	void addDiagonalsInNaturalBasis(SparseMatrixType &hmatrix,
	                                const VectorOperatorType& cm,
	                                const BlockType& block,
	                                RealType time,
	                                RealType factorForDiagonals=1.0)  const
	{
		SizeType n=block.size();
		SparseMatrixType tmpMatrix,tmpMatrix2,niup,nidown;
		SizeType linSize = geometry_.numberOfSites();

		for (SizeType i=0;i<n;i++) {

			// V_iup term
			tmp = modelParameters_.potentialV[block[i]]*factorForDiagonals;
			multiplyScalar(tmpMatrix,niup,static_cast<SparseElementType>(tmp));
			hmatrix += tmpMatrix;

			if (modelParameters_.potentialT.size()==0) continue;
			RealType cosarg = cos(time*modelParameters_.omega +
			                      modelParameters_.phase);
			// VT_iup term
			tmp = modelParameters_.potentialT[block[i]]*factorForDiagonals;
			tmp *= cosarg;
			multiplyScalar(tmpMatrix,niup,static_cast<SparseElementType>(tmp));
			hmatrix += tmpMatrix;
		}
	}

	virtual const TargetQuantumElectronsType& targetQuantum() const
	{
		return modelParameters_.targetQuantum;
	}

private:

	// Calculate fermionic sign when applying operator
	// c^\dagger_{i\sigma} to basis state ket
	RealType sign(typename HilbertSpaceHubbardType::HilbertState const &ket,
	              int i,
	              int sigma) const
	{
		int value=0;
		value += HilbertSpaceHubbardType::calcNofElectrons(ket,0,i,0);
		int tmp1 = HilbertSpaceHubbardType::get(ket,0) &1;
		if (i>0 && tmp1>0) value++;

		return (value%2==0) ? 1.0 : FERMION_SIGN;
	}

	//! Find c^\dagger_isigma in the natural basis natBasis
	SparseMatrixType findOperatorMatrices(int i,
	                                      int sigma,
	                                      const HilbertBasisType& natBasis) const
	{
		typename HilbertSpaceHubbardType::HilbertState bra,ket;
		int n = natBasis.size();
		PsimagLite::Matrix<typename SparseMatrixType::value_type> cm(n,n);

		for (SizeType ii=0;ii<natBasis.size();ii++) {
			bra=ket=natBasis[ii];
			if (HilbertSpaceHubbardType::isNonZero(ket,i,sigma)) {

			} else {
				HilbertSpaceHubbardType::create(bra,i,sigma);
				int jj = PsimagLite::isInVector(natBasis,bra);
				assert(jj >= 0);
				cm(ii,jj) =sign(ket,i,sigma);
			}
		}

		SparseMatrixType creationMatrix(cm);
		return creationMatrix;
	}

	void findQuantumNumbers(typename PsimagLite::Vector<SizeType>::Type& q,
	                        const HilbertBasisType& basis,
	                        int n) const
	{
		SymmetryElectronsSzType qq;
		setSymmetryRelated(qq,basis,n);
		qq.findQuantumNumbers(q, MyBasis::useSu2Symmetry());
	}

	void setSymmetryRelated(SymmetryElectronsSzType& q,
	                        HilbertBasisType  const &basis,int) const
	{
		// find j,m and flavors (do it by hand since we assume n==1)
		// note: we use 2j instead of j
		// note: we use m+j instead of m
		// This assures us that both j and m are SizeType
		typedef std::pair<SizeType,SizeType> PairType;
		typename PsimagLite::Vector<PairType>::Type jmvalues;
		typename PsimagLite::Vector<SizeType>::Type flavors;
		PairType jmSaved = calcJmValue<PairType>(basis[0]);
		jmSaved.first++;
		jmSaved.second++;

		typename PsimagLite::Vector<SizeType>::Type electronsUp(basis.size());
		typename PsimagLite::Vector<SizeType>::Type zero(basis.size(),0);
		for (SizeType i=0;i<basis.size();i++) {
			PairType jmpair = calcJmValue<PairType>(basis[i]);

			jmvalues.push_back(jmpair);
			// nup
			electronsUp[i] = HilbertSpaceHubbardType::getNofDigits(basis[i],SPIN_UP);

			flavors.push_back(electronsUp[i]);
			jmSaved = jmpair;
		}

		q.set(jmvalues,flavors,electronsUp,zero);
	}

	// note: we use 2j instead of j
	// note: we use m+j instead of m
	// This assures us that both j and m are SizeType
	// does not work for 6 or 9
	template<typename PairType>
	PairType calcJmValue(const HilbertState& ket) const
	{
		SizeType site0=0;
		SizeType site1=0;

		spinSquared_.doOnePairOfSitesA(ket,site0,site1);
		spinSquared_.doOnePairOfSitesB(ket,site0,site1);
		spinSquared_.doDiagonal(ket,site0,site1);

		RealType sz = spinSquared_.spinZ(ket,site0);
		PairType jm= spinSquaredHelper_.getJmPair(sz);

		return jm;
	}

	//serializr start class FermionSpinless
	//serializr vptr
	//serializr normal modelParameters_
	ParametersFermionSpinless<RealType>  modelParameters_;
	//serializr ref geometry_
	const GeometryType &geometry_;
	//serializr normal offset_
	SizeType offset_;
	//serializr normal spinSquaredHelper_
	SpinSquaredHelper<RealType,WordType> spinSquaredHelper_;
	//serializr normal spinSquared_
	SpinSquared<SpinSquaredHelper<RealType,WordType> > spinSquared_;
};	//class FermionSpinless

} // namespace Dmrg
/*@}*/
#endif

