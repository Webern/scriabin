#include "pen/Coalescence.h"
#include "pen/Prob.h"
#include "../test/PEN_PATH.h"
#include "mx/api/DocumentManager.h"
#include <vector>
#include <map>

namespace pen
{
    static constexpr const int BEATS_PER_MEASURE = 6;
    static constexpr const int BEAT_TYPE_NUMERAL = 8;
    static constexpr const int QUARTER_NOTE_BEAT_TYPE_NUMERAL = 4;
    static constexpr const mx::api::DurationName BEAT_TYPE_NAME = mx::api::DurationName::eighth;
    static constexpr const int TICKS_PER_QUARTER = 2;
    static constexpr const double BEAT_TYPE_TO_QUARTER_RATIO = static_cast<double>( QUARTER_NOTE_BEAT_TYPE_NUMERAL ) / static_cast<double>( BEAT_TYPE_NUMERAL );
    static constexpr const int TICKS_PER_BEAT = static_cast<int>( ( static_cast<double>( TICKS_PER_QUARTER ) * BEAT_TYPE_TO_QUARTER_RATIO ) + 0.0000000001 );
    
    Coalescence::Coalescence( std::string inputFilepath, std::string outputFilepath )
    : myScore{}
    , myInFilepath{ std::move( inputFilepath ) }
    , myOutFilepath{ std::move( outputFilepath ) }
    {

    }
    
    
    void
    Coalescence::initSelfScore()
    {
        myScore = mx::api::ScoreData{};
        myScore = createEmptyScore( "Coalescence" );
    }
    
    
    MxNoteStreams
    Coalescence::getInputNotes() const
    {
        auto& dmgr = mx::api::DocumentManager::getInstance();
        const auto inID = dmgr.createFromFile( myInFilepath );
        const auto input = dmgr.getData( inID );
        dmgr.destroyDocument( inID );
        MxNoteStreams inputNotes;
        int partIndex = 0;
        
        for( const auto& part : input.parts )
        {
            for( const auto& measure : part.measures )
            {
                const auto& staff = measure.staves.at( 0 );
                const auto& voice = staff.voices.at( 0 );
                const auto& notes = voice.notes;
                
                for( const auto& note : notes )
                {
                    inputNotes[partIndex].push_back( note );
                }
            }
            ++partIndex;
        }
        
        return inputNotes;
    }
    
    
    AtomStreams
    Coalescence::extractStreams( const MxNoteStreams& inNotes )
    {
        AtomStreams streams;
        
        for( const auto& origPair : inNotes )
        {
            for( const auto& note : origPair.second )
            {
                Atom a;
                if( !note.isRest )
                {
                    a.setFromMx( note.pitchData );
                }
                
                int numNotes = 1;
                
                if( note.durationData.durationName == mx::api::DurationName::quarter )
                {
                    numNotes = 2;
                    if( note.durationData.durationDots == 1 )
                    {
                        numNotes = 3;
                    }
                }
                
                a.updateName();
                
                for( int i = 0; i < numNotes; ++i )
                {
                    streams[origPair.first].push_back( a );
                }
            }
        }
        
        // fix a problem with the viola part by copying violin 1
        // and transposing it down two octaves
        streams.at( 2 ) = streams.at( 0 );
        
        for( auto& note : streams.at( 2 ) )
        {
            note.setOctave( note.getOctave() - 1 );
        }
        
        return streams;
    }
    
    
    void
    Coalescence::reverseAtoms( Atoms& ioAtoms )
    {
        std::reverse(std::begin( ioAtoms ), std::end( ioAtoms ) );
    }
    
    
    void
    Coalescence::reverseStreams( AtomStreams& ioStreams )
    {
        for( auto& pair : ioStreams )
        {
            reverseAtoms( pair.second );
        }
    }
    
    
    void
    Coalescence::writeMusic( const Atoms& inAtomsToWrite, Atoms& ioAtomsToAppendTo, int numTimes )
    {
        for( int i = 0; i < numTimes; ++i )
        {
            std::copy( std::cbegin( inAtomsToWrite ), std::cend( inAtomsToWrite ), std::back_inserter( ioAtomsToAppendTo ) );
        }
    }
    
    
    void
    Coalescence::writeMusic( const AtomStreams& inStreamsToWrite, AtomStreams& ioStreamsToAppendTo, int numTimes )
    {
        if( inStreamsToWrite.size() != ioStreamsToAppendTo.size() )
        {
            std::runtime_error( "Coalescence::writeMusic: maps must be same size" );
        }
        
        auto inIter = inStreamsToWrite.cbegin();
        auto outIter = ioStreamsToAppendTo.begin();
        const auto e = inStreamsToWrite.cend();
        
        for( ; inIter != e; ++inIter, ++outIter )
        {
            writeMusic( inIter->second, outIter->second, numTimes );
        }
    }
    
    
    void
    Coalescence::doSomeAwesomeCoalescing( const AtomStreams& inOriginalMusic,
                                          AtomStreams& ioPatternStreams,
                                          AtomStreams& ioOutputStreams,
                                          Prob& ioProb )
    {
        CoalescenceParams params;
        params.minR = 0;
        params.maxR = 0;
        params.rInc = 0;
        params.rTier = 0;
        
        params.minP = 1;
        params.maxP = 1;
        params.pInc = 0;
        params.pTier = 1;
        params.numLoops = 3;
        
        doCoalescingLoop( params, ioPatternStreams, ioOutputStreams, ioProb );
        writeMusic( ioPatternStreams, ioOutputStreams, 3 );
        
        params.minR = 0;
        params.maxR = 0;
        params.rInc = 0;
        params.rTier = 0;
        
        params.minP = 1;
        params.maxP = 1;
        params.pInc = 0;
        params.pTier = 1;
        params.numLoops = 2;
        
        doCoalescingLoop( params, ioPatternStreams, ioOutputStreams, ioProb );
        writeMusic( ioPatternStreams, ioOutputStreams, 2 );
        
        params.minR = 0;
        params.maxR = 0;
        params.rInc = 0;
        params.rTier = 0;
        
        params.minP = 1;
        params.maxP = 1;
        params.pInc = 0;
        params.pTier = 1;
        params.numLoops = 2;
        
        doCoalescingLoop( params, ioPatternStreams, ioOutputStreams, ioProb );
        writeMusic( ioPatternStreams, ioOutputStreams, 2 );
        
        // add a rest to viola
        int restPartIndex = 2;
        auto restIndex = findInsertIndex( ioPatternStreams.at( restPartIndex ), ioProb );
        auto restIter = ioPatternStreams.at( restPartIndex ).cbegin() + static_cast<ptrdiff_t>( restIndex );
        restIter = ioPatternStreams.at( restPartIndex ).insert( restIter, Atom{} );
        
        params.minR = 0;
        params.maxR = 0;
        params.rInc = 0;
        params.rTier = 0;
        
        params.minP = 1;
        params.maxP = 1;
        params.pInc = 0;
        params.pTier = 1;
        params.numLoops = 2;
        
        doCoalescingLoop( params, ioPatternStreams, ioOutputStreams, ioProb );
        writeMusic( ioPatternStreams, ioOutputStreams, 2 );
        
        // add a rest to violin 2
        restPartIndex = 1;
        restIndex = findInsertIndex( ioPatternStreams.at( restPartIndex ), ioProb );
        restIter = ioPatternStreams.at( restPartIndex ).cbegin() + static_cast<ptrdiff_t>( restIndex );
        restIter = ioPatternStreams.at( restPartIndex ).insert( restIter, Atom{} );
        
        params.minR = 0;
        params.maxR = 0;
        params.rInc = 0;
        params.rTier = 0;
        
        params.minP = 3;
        params.maxP = 3;
        params.pInc = 0;
        params.pTier = 1;
        params.numLoops = 2;
        
        doCoalescingLoop( params, ioPatternStreams, ioOutputStreams, ioProb );
        writeMusic( ioPatternStreams, ioOutputStreams, 2 );
        
        // add a rest to violin 1
        restPartIndex = 0;
        restIndex = findInsertIndex( ioPatternStreams.at( restPartIndex ), ioProb );
        restIter = ioPatternStreams.at( restPartIndex ).cbegin() + static_cast<ptrdiff_t>( restIndex );
        restIter = ioPatternStreams.at( restPartIndex ).insert( restIter, Atom{} );
        restIter = ioPatternStreams.at( restPartIndex ).insert( restIter, Atom{} );
        
        params.minR = 0;
        params.maxR = 0;
        params.rInc = 0;
        params.rTier = 0;
        
        params.minP = 3;
        params.maxP = 3;
        params.pInc = 0;
        params.pTier = 1;
        params.numLoops = 2;
        
        doCoalescingLoop( params, ioPatternStreams, ioOutputStreams, ioProb );
        // writeMusic( ioPatternStreams, ioOutputStreams, 2 );
        
        // add a rest to cello
        restPartIndex = 3;
        restIndex = findInsertIndex( ioPatternStreams.at( restPartIndex ), ioProb );
        restIter = ioPatternStreams.at( restPartIndex ).cbegin() + static_cast<ptrdiff_t>( restIndex );
        restIter = ioPatternStreams.at( restPartIndex ).insert( restIter, Atom{} );
        restIter = ioPatternStreams.at( restPartIndex ).insert( restIter, Atom{} );
        
        params.minR = 0;
        params.maxR = 0;
        params.rInc = 0;
        params.rTier = 0;
        
        params.minP = 10;
        params.maxP = 20;
        params.pInc = 1;
        params.pTier = 1;
        params.numLoops = 5;
        
        doCoalescingLoop( params, ioPatternStreams, ioOutputStreams, ioProb );
        const int NUM_EXPANSIONS_PER_PHRASE = 3;
        const int NUM_TIMES_THROUGH_THE_PHRASE = 10;
        
        for( int loop = 0; loop < NUM_TIMES_THROUGH_THE_PHRASE; ++loop )
        {
            for( int add = 0; add < NUM_EXPANSIONS_PER_PHRASE; ++add )
            {
                for( auto& p : ioPatternStreams )
                {
                    expandShortestReps( p.second, ioProb );
                }
                
            }
            writeMusic( ioPatternStreams, ioOutputStreams, 1 );
        }
        
        
        
        params.minR = 0;
        params.maxR = 0;
        params.rInc = 0;
        params.rTier = 0;
        
        params.minP = 33;
        params.maxP = 33;
        params.pInc = 0;
        params.pTier = 5;
        params.numLoops = 5;
        
        doCoalescingLoop( params, ioPatternStreams, ioOutputStreams, ioProb );
    }
    
    
    inline int
    chooseAtRandom( const std::set<int>::const_iterator beginIter, const std::set<int>::const_iterator endIter, Prob& ioProb )
    {
        if( beginIter == endIter )
        {
            return -1;
        }
        
        int escapeHatch = 0;
        std::set<int>::const_iterator it = beginIter;
        
        for( ; escapeHatch < 100000; ++escapeHatch )
        {
            if( ioProb.get( 30 ) )
            {
                return *it;
            }
            
            ++it;
            
            if( it == endIter )
            {
                it = beginIter;
            }
        }
        
        return -1;
    }
    
    
    void
    Coalescence::expandShortestReps( Atoms& ioPattern,
                                     Prob& ioProb )
    {
        const auto nonReps = findNonRepeatingNotes( ioPattern );
        
        if( !nonReps.empty() )
        {
            if( nonReps.empty() )
            {
                return;
            }
            
            const int rando = chooseAtRandom( std::cbegin( nonReps ), std::cend( nonReps ), ioProb );
            
            if( rando < 0 || rando >= static_cast<int>( ioPattern.size() ) )
            {
                throw std::runtime_error{ "not good" };
            }
            
            const auto randoIter = ioPattern.cbegin() + static_cast<ptrdiff_t>( rando );
            
//            if( randoIter->getStep() == -1 )
//            {
//                std::cout << "expanding a rest" << std::endl;
//            }
//            else
//            {
//                std::cout << "expanding a note" << std::endl;
//            }
            
            ioPattern.insert( randoIter, *randoIter );
            return;
        }

        const auto reps = findRepeatedNotes( ioPattern );
        std::vector<AtomPattern> sorted;

        for( const auto& pair : reps )
        {
            sorted.push_back( pair.second );
        }
        
        const auto compare = [&]( AtomPattern& l, AtomPattern& r )
        {
            if( l.patternLength < r.patternLength )
            {
                return true;
            }
            else if( l.patternLength > r.patternLength )
            {
                return false;
            }
            else if( l.firstAtomOfPattern.getOctave() < r.firstAtomOfPattern.getOctave() )
            {
                return true;
            }
            else if( l.firstAtomOfPattern.getOctave() > r.firstAtomOfPattern.getOctave() )
            {
                return false;
            }
            else if( l.firstAtomOfPattern.getStep() < r.firstAtomOfPattern.getStep() )
            {
                return true;
            }
            else if( l.firstAtomOfPattern.getStep() > r.firstAtomOfPattern.getStep() )
            {
                return false;
            }
            else if( l.firstAtomOfPattern.getAlter() < r.firstAtomOfPattern.getAlter() )
            {
                return true;
            }
            else if( l.firstAtomOfPattern.getAlter() > r.firstAtomOfPattern.getAlter() )
            {
                return false;
            }
            
            return false;
        };
        
        std::sort( std::begin( sorted ), std::end( sorted ), compare );
        const auto& first = sorted.front();
        const auto insertIndex = first.index;
        const auto insertIter = ioPattern.cbegin() + static_cast<ptrdiff_t>( insertIndex );
//        if( insertIter->getStep() == -1 )
//        {
//            std::cout << "expanding a rest" << std::endl;
//        }
//        else
//        {
//            std::cout << "expanding a note" << std::endl;
//        }
        ioPattern.insert( insertIter, *insertIter );
    }
    
    
    int
    Coalescence::findInsertIndex( const Atoms& inAtoms, Prob& ioProb )
    {
        // make a datastructure showing where the repetitions and rests are
        const auto repetitions = findRepeatedNotes( inAtoms );
        
        // find indices of all repeated notes and non-repeated notes
        std::set<int> repeatedIndices;
        std::set<int> nonRepeatedIndices;
        
        for( const auto& r : repetitions )
        {
            for( int x = r.second.index; x < r.second.index + r.second.patternLength; ++x )
            {
                repeatedIndices.insert( x );
            }
        }
        
        for( int x = 0; x < static_cast<int>( inAtoms.size() ); ++x )
        {
            if( repeatedIndices.find( x ) == repeatedIndices.cend() )
            {
                nonRepeatedIndices.insert( x );
            }
        }
        
        // find a non-repeated index, or if none exist, then an index where there
        // is less than 3 repitions, or if none exists then a random index
        int insertIndex = -1;
        
        if( repeatedIndices.size() > 0 )
        {
            const auto& collection = repeatedIndices;
            const auto begit = collection.cbegin();
            const auto endit = collection.cend();
            auto iter = begit;
            
            while( insertIndex < 0 )
            {
                if( ioProb.get( 1 ) )
                {
                    insertIndex = *iter;
                }
                
                ++iter;
                
                if( iter == endit )
                {
                    iter = begit;
                }
            }
        }
        else
        {
            const auto& collection = nonRepeatedIndices;
            const auto begit = collection.cbegin();
            const auto endit = collection.cend();
            auto iter = begit;
            
            while( insertIndex < 0 )
            {
                if( ioProb.get( 1 ) )
                {
                    insertIndex = *iter;
                }
                
                ++iter;
                
                if( iter == endit )
                {
                    iter = begit;
                }
            }
        }
        
        if( insertIndex < 0 || insertIndex >= static_cast<int>( inAtoms.size() ) )
        {
            throw std::runtime_error{ "index will be out of range" };
        }
        
        return insertIndex;
    }
    
    
    void
    Coalescence::doEverthing()
    {
        initSelfScore();
        const MxNoteStreams inputNotes = getInputNotes();
        const AtomStreams originalMusic = extractStreams( inputNotes );
        AtomStreams outMusic = originalMusic;
        reverseStreams( outMusic );
        const AtomStreams originalReversedMusic = outMusic;
        AtomStreams patternStreams = originalReversedMusic;
        Prob boolGen{ DIGITS_DAT_PATH() };
        doSomeAwesomeCoalescing( originalMusic, patternStreams, outMusic, boolGen );
        shortenStreamsToMatchLengthOfShortestStream( outMusic, BEATS_PER_MEASURE );
        reverseStreams( outMusic );
        writeMusic( originalMusic, outMusic, 32 );
        augmentBeginning( outMusic );
        
        writeStreamsToScore( outMusic, myScore );
        auto& dmgr = mx::api::DocumentManager::getInstance();
        const auto oID = dmgr.createFromScore( myScore );
        dmgr.writeToFile( oID, myOutFilepath );
    }
    
    
    void
    Coalescence::doCoalescingLoop( const CoalescenceParams& params,
                                   AtomStreams& ioPatternStreams,
                                   AtomStreams& ioOutputStreams,
                                   Prob& ioProb )
    {
        int rCurrent = params.minR;
        int pCurrent = params.minP;
        
        for( int loopCounter = 0; loopCounter < params.numLoops; ++loopCounter )
        {
            for( int p = 0; p < 4; ++p )
            {
                const int restProb = rCurrent + ( params.rTier * p );
                const int repeatProb = pCurrent + ( params.pTier * p );
                
                for( auto it = ioPatternStreams.at( p ).begin(); it != ioPatternStreams.at( p ).end(); ++it )
                {
                    const bool doRest = ioProb.get( restProb );
                    const bool doRepeat = ioProb.get( repeatProb );
                    
                    if( doRest )
                    {
                        const auto currentIndex = it - ioPatternStreams.at( p ).begin();
                        const auto insertIndex = findInsertIndex( ioPatternStreams.at( p ), ioProb );
                        const auto insertIter = ioPatternStreams.at( p ).cbegin() + static_cast<ptrdiff_t>( insertIndex );
                        ioPatternStreams.at( p ).insert( insertIter, Atom{} );
                        it = ioPatternStreams.at( p ).begin() + currentIndex;
                    }
                    
                    if( doRepeat )
                    {
                        it = ioPatternStreams.at( p ).insert( it, *it );
                    }
                }
            }
            
            writeMusic( ioPatternStreams, ioOutputStreams, 1 );
            
            rCurrent += params.rInc;
            pCurrent += params.pInc;
            
            if( rCurrent > params.maxR )
            {
                rCurrent = params.maxR;
            }
            
            if( pCurrent > params.maxP )
            {
                rCurrent = params.maxP;
            }
        }
    }
    
    void
    Coalescence::shortenStreamsToMatchLengthOfShortestStream( AtomStreams& ioStreams, int inMultipleOf )
    {
        int smallest = findIndexOfShortestStream( ioStreams );
        const int originalSizeSmallest = static_cast<int>( ioStreams.at( smallest ).size() );
        
        if( inMultipleOf > 0 && originalSizeSmallest > 0 && originalSizeSmallest % inMultipleOf != 0 )
        {
            const int remainder = originalSizeSmallest % inMultipleOf;
            const auto newSize = ioStreams.at( smallest ).size() - static_cast<size_t>( remainder );
            ioStreams.at( smallest ).resize( newSize );
        }
        
        const auto sizeOfSmallest = ioStreams.at( smallest ).size();
        
        // delete extra notes based on smallest
        for( auto& pair : ioStreams )
        {
            if( pair.second.size() > sizeOfSmallest )
            {
                pair.second.resize( sizeOfSmallest );
            }
        }
    }
    
    
    int
    Coalescence::findIndexOfShortestStream( const AtomStreams& inStreams )
    {
        // find the smallest stream
        int smallest = -1;
        int indexOfSmallest = 0;
        int currentIndex = 0;
        
        for( const auto& streamPair : inStreams )
        {
            if( smallest == -1 || static_cast<int>( streamPair.second.size() ) < smallest )
            {
                smallest = static_cast<int>( streamPair.second.size() );
                indexOfSmallest = currentIndex;
            }
            ++currentIndex;
        }
        
        return indexOfSmallest;
    }
    
    
    void
    Coalescence::writeStreamsToScore( const AtomStreams& inStreams, mx::api::ScoreData& ioScore )
    {
        for( int p = 0; p < static_cast<int>( inStreams.size() ); ++p )
        {
            int measureIndex = 0;
            int beatIndex = 0;
            const auto& noteStream = inStreams.at( p );
            writeStream( p, measureIndex, beatIndex, noteStream, ioScore );
        }
    }
    
    
    void
    Coalescence::writeStream( int partIndex,
                              int startingMeasureIndex,
                              int startingEighthIndex,
                              const Atoms& inAtoms,
                              mx::api::ScoreData& ioScore )
    {
        auto measureIndex = startingMeasureIndex;
        auto beatIndex = startingEighthIndex;
        
        for( const auto& atom : inAtoms )
        {
            auto& part = ioScore.parts.at( static_cast<size_t>( partIndex) );
            
            while( measureIndex > static_cast<int>( part.measures.size() ) - 1 )
            {
                appendMeasures( ioScore, 1 );
            }
            
            auto& measure = part.measures.at( static_cast<size_t>( measureIndex ) );
            mx::api::NoteData theNote;
            theNote.tickTimePosition = beatIndex * TICKS_PER_BEAT;
            theNote.durationData.durationTimeTicks = TICKS_PER_BEAT;
            theNote.durationData.durationName = BEAT_TYPE_NAME;
            
            if( atom.getStep() == -1 )
            {
                theNote.isRest = true;
            }
            else
            {
                theNote.pitchData = atom.getMxPitchData();
            }
            
            measure.staves.at( 0 ).voices.at( 0 ).notes.push_back( theNote );
            
            if( ( beatIndex + 1 ) % BEATS_PER_MEASURE == 0 )
            {
                ++measureIndex;
                beatIndex = 0;
            }
            else
            {
                ++beatIndex;
            }
        }
    }
    
    
    mx::api::ScoreData
    Coalescence::createEmptyScore( const std::string& title )
    {
        mx::api::ScoreData score;
        score.ticksPerQuarter = TICKS_PER_QUARTER;
        score.workTitle = title;
        mx::api::ClefData treble;
        treble.setTreble();
        mx::api::ClefData alto;
        alto.setAlto();
        mx::api::ClefData bass;
        bass.setBass();
        addInstrument( score, "VN1", "Violin 1", "Vln 1", mx::api::SoundID::stringsViolin, treble );
        addInstrument( score, "VN2", "Violin 2", "Vln 2", mx::api::SoundID::stringsViolin, treble );
        addInstrument( score, "VLA", "Viola", "Vla", mx::api::SoundID::stringsViola, alto );
        addInstrument( score, "VLC", "Cello", "Vlc", mx::api::SoundID::stringsCello, bass );
        mx::api::PartGroupData grp;
        grp.firstPartIndex = 0;
        grp.lastPartIndex = 3;
        grp.bracketType = mx::api::BracketType::bracket;
        score.partGroups.push_back( grp );
        return score;
    }
    
    
    void
    Coalescence::addInstrument( mx::api::ScoreData& ioScore,
                          const std::string& id,
                          const std::string& name,
                          const std::string& abbr,
                          mx::api::SoundID soundID,
                          const mx::api::ClefData clef  )
    {
        ioScore.parts.emplace_back();
        ioScore.parts.back().uniqueId = id;
        ioScore.parts.back().name = name;
        ioScore.parts.back().abbreviation = abbr;
        ioScore.parts.back().instrumentData.uniqueId = ioScore.parts.back().uniqueId + "-I";
        ioScore.parts.back().displayName = ioScore.parts.back().name;
        ioScore.parts.back().displayAbbreviation = ioScore.parts.back().abbreviation;
        ioScore.parts.back().instrumentData.soundID = soundID;
        ioScore.parts.back().instrumentData.soloOrEnsemble = mx::api::SoloOrEnsemble::solo;
        
        ioScore.parts.back().measures.emplace_back();
        ioScore.parts.back().measures.back().staves.emplace_back();
        mx::api::TimeSignatureData tsd;
        tsd.beats = BEATS_PER_MEASURE;
        tsd.beatType = BEAT_TYPE_NUMERAL;
        tsd.isImplicit = false;
        ioScore.parts.back().measures.back().timeSignature = tsd;
        ioScore.parts.back().measures.back().staves.back().clefs.push_back( clef );
        ioScore.parts.back().measures.back().staves.back().voices[0] = mx::api::VoiceData{};
    }
    
    
    void
    Coalescence::appendMeasures( mx::api::ScoreData& ioScore, int numMeasures )
    {
        for( int x = 0; x < numMeasures; ++x )
        {
            for( size_t partIndex = 0; partIndex < ioScore.parts.size(); ++partIndex )
            {
                const auto lastMeasure = ioScore.parts.at( partIndex ).measures.back();
                mx::api::MeasureData prototype;
                prototype.timeSignature = lastMeasure.timeSignature;
                prototype.timeSignature.isImplicit = true;
                
                for( const auto& staff : lastMeasure.staves )
                {
                    prototype.staves.emplace_back();
                    for( const auto& voicePair : staff.voices )
                    {
                        prototype.staves.back().voices[voicePair.first] = mx::api::VoiceData{};
                    }
                }
                
                ioScore.parts.at( partIndex ).measures.push_back( prototype );
            }
        }
    }
    
    
    void
    Coalescence::augmentBeginning( AtomStreams& ioOutMusic )
    {
        std::vector<int> changeIndices;
        std::vector<Atom> previousNotes;
        std::vector<Atom> currentNotes;
        int noteIndex = 0;
        const int lastNote = static_cast<int>( ioOutMusic.at( 0 ).size() ) - 1;
        
        for( ; noteIndex <= lastNote && changeIndices.size() < 8; ++noteIndex )
        {
            currentNotes.clear();
            
            for( const auto& p : ioOutMusic )
            {
                const auto a = p.second.at( static_cast<size_t>( noteIndex) );
                currentNotes.push_back( a );
            }
            
            if( previousNotes.empty() )
            {
                changeIndices.push_back( noteIndex );
            }
            else
            {
                if( previousNotes.size() != currentNotes.size() )
                {
                    throw std::runtime_error{ "previousNotes.size() != currentNotes.size()" };
                }
                
                auto prevIter = previousNotes.cbegin();
                auto currIter = currentNotes.cbegin();
                const auto prevEnd = previousNotes.cend();
                
                for( ; prevIter != prevEnd; ++prevIter, ++currIter )
                {
                    if( *prevIter != *currIter )
                    {
                        changeIndices.push_back( noteIndex - 1 );
                    }
                }
            }
            
            previousNotes = currentNotes;
        }
        
        int changeInstance = static_cast<int>( changeIndices.size() );
        for( auto changeIndexIter = changeIndices.crbegin(); changeIndexIter != changeIndices.crend(); ++changeIndexIter, --changeInstance )
        {
            const auto changeIndex = *changeIndexIter;
            const auto notesToAdd = 18;//;changeInstance * BEATS_PER_MEASURE;
            
            for( auto& p : ioOutMusic )
            {
                auto insertIter = p.second.cbegin() + static_cast<ptrdiff_t>( changeIndex );
                
                for( int z = 0; z < notesToAdd; ++z )
                {
                    insertIter = p.second.insert( insertIter, *insertIter );
                }
            }
        }
    }
}
