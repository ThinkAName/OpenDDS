// -*- C++ -*-
//
// $Id$

#include "DcpsInfo_pch.h"
#include "dds/DCPS/debug.h"
#include "FederatorConfig.h"
#include "FederatorC.h"
#include "ace/Configuration.h"
#include "ace/Configuration_Import_Export.h"
#include "ace/Log_Priority.h"
#include "ace/Log_Msg.h"

#if !defined (__ACE_INLINE__)
# include "FederatorConfig.inl"
#endif /* ! __ACE_INLINE__ */

namespace { // Anonymous namespace for file scope

  // FederationDomain key value.
  const ACE_TCHAR FEDERATION_DOMAIN_KEY[] = ACE_TEXT("FederationDomain");

  // FederationId key value.
  const ACE_TCHAR FEDERATION_ID_KEY[] = ACE_TEXT("FederationId");

  // FederationId key value.
  const ACE_TCHAR FEDERATION_PORT_KEY[] = ACE_TEXT("FederationPort");

  /// Define an argument copying functor.
  class ArgCopier {
    public:
      /// Construct with a target pointer array.
      ArgCopier( char** target_, std::string& configFile);

      /// The Functor function operator.
      void operator()( char* arg);

    private:
      /// The copy target.
      char** target_;

      /// The configuration filename target.
      std::string& configFile_;

      /// Ugly argument value switch.
      bool fileArg_;
  };

  ArgCopier::ArgCopier( char** target, std::string& configFile)
   : target_( target),
     configFile_( configFile),
     fileArg_( false)
  {
  }

  void
  ArgCopier::operator()( char* arg)
  {
    // Handle the file argument and its value.
    if( ::OpenDDS::Federator::Config::FEDERATOR_CONFIG_OPTION == arg) {
      // Configuration file option, filename value is next arg.
      this->fileArg_ = true;
      return;
    }

    if( this->fileArg_ == true) {
      // Store the configuration file name, but don't copy the arg.
      this->configFile_ = arg;

    } else {
      // Copy other args verbatim.
      *this->target_++ = arg;
    }
    this->fileArg_ = false;
  }

} // End of anonymous namespace

namespace OpenDDS { namespace Federator {

const std::string
Config::FEDERATOR_CONFIG_OPTION( "-FederatorConfig");

Config::Config( int argc, char** argv)
 : federationId_( NIL_REPOSITORY),
   federationDomain_( DEFAULT_FEDERATIONDOMAIN),
   federationPort_( -1)
{
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: Federator::Config::Config()\n")
    ));
  }

  // Remove one option/value pair, add one option/value pair.
  int additionalSlots = 0;

  // Setup the internal storage.
  //
  // N.B. We will be adding two arguments and their respective values,
  //      but we will also be removing the configuration file option and
  //      its value for a total of 2 additional slots.  If there is no
  //      configuration file option, we will not be removing any
  //      arguments, but neither will we be adding any.
  //
  this->argc_ = argc;
  this->argv_ = new char*[ argc + additionalSlots];

  // Copy the existing arguments verbatim.
  ArgCopier argCopier( this->argv_, this->configFile_);
  std::for_each( &argv[0], &argv[ argc], argCopier);

  // Read and process any configuration file.
  this->processFile();
}

Config::~Config()
{
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: Federator::Config::~FederatorConfig()\n")
    ));
  }

  // We prwn this
  delete [] this->argv_;
}

void
Config::processFile()
{
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: Federator::Config::process()\n")
    ));
  }

  if( this->configFile_.empty()) {
    // No filename, no processing.
    return;
  }

  // Grab a spot to stick the configuration.
  ACE_Configuration_Heap heap;
  if( 0 != heap.open()) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Federator::Config::process - ")
      ACE_TEXT("unable to open configuration heap.\n")
    ));
    return;
  }

  // Import the file into our shiny new spot.
  ACE_Ini_ImpExp import( heap);
  if( 0 != import.import_config( this->configFile_.c_str())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Federator::Config::process - ")
      ACE_TEXT("unable to import configuration file.\n")
    ));
    return;
  }

  // Configuration file format:
  //
  //   FederationDomain = <number>                       (REQUIRED)
  //   FederationId     = <number>                       (REQUIRED)
  //   FederationPort   = <number>                       (REQUIRED)
  //

  // Grab the common configuration settings.
  const ACE_Configuration_Section_Key &root = heap.root_section();

  // Federation Domain value - REQUIRED
  ACE_TString federationDomainString;
  if( 0 != heap.get_string_value( root, FEDERATION_DOMAIN_KEY, federationDomainString)) {
    ACE_ERROR(( LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Federator::Config::process - ")
      ACE_TEXT("Unable to obtain value for FederationDomain in root section\n")
    ));
    return;
  }

  // Convert to numeric repository key value.
  this->federationDomain_ = ACE_OS::atoi( federationDomainString.c_str());
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t)   FederationDomain == %d\n"),
      this->federationDomain_
    ));
  }

  // Federation Id value - REQUIRED
  ACE_TString federationIdString;
  if( 0 != heap.get_string_value( root, FEDERATION_ID_KEY, federationIdString)) {
    ACE_ERROR(( LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Federator::Config::process - ")
      ACE_TEXT("Unable to obtain value for FederationId in root section\n")
    ));
    return;
  }

  // Convert to numeric repository key value.
  this->federationId_ = ACE_OS::atoi( federationIdString.c_str());
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t)   FederationId == %d\n"),
      this->federationId_
    ));
  }

  // Federation port value - REQUIRED
  ACE_TString federationPortString;
  if( 0 != heap.get_string_value( root, FEDERATION_PORT_KEY, federationPortString)) {
    ACE_ERROR(( LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Federator::Config::process - ")
      ACE_TEXT("Unable to obtain value for FederationPort in root section\n")
    ));
    return;
  }

  // Convert to numeric repository key value.
  this->federationPort_ = ACE_OS::atoi( federationPortString.c_str());
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t)   FederationPort == %d\n"),
      this->federationPort_
    ));
  }

}

}} // End namespace OpenDDS::Federator

