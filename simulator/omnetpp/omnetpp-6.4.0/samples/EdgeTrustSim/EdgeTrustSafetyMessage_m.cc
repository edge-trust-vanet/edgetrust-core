//
// Generated file, do not edit! Created by opp_msgtool 6.4 from veins/modules/application/edgetrust/EdgeTrustSafetyMessage.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wshadow"
#  pragma clang diagnostic ignored "-Wconversion"
#  pragma clang diagnostic ignored "-Wunused-parameter"
#  pragma clang diagnostic ignored "-Wc++98-compat"
#  pragma clang diagnostic ignored "-Wunreachable-code-break"
#  pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined(__GNUC__)
#  pragma GCC diagnostic ignored "-Wshadow"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"
#  pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif

#include <iostream>
#include <sstream>
#include <memory>
#include <type_traits>
#include "EdgeTrustSafetyMessage_m.h"

namespace omnetpp {

// Template pack/unpack rules. They are declared *after* a1l type-specific pack functions for multiple reasons.
// They are in the omnetpp namespace, to allow them to be found by argument-dependent lookup via the cCommBuffer argument

// Packing/unpacking an std::vector
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::vector<T,A>& v)
{
    int n = v.size();
    doParsimPacking(buffer, n);
    for (int i = 0; i < n; i++)
        doParsimPacking(buffer, v[i]);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::vector<T,A>& v)
{
    int n;
    doParsimUnpacking(buffer, n);
    v.resize(n);
    for (int i = 0; i < n; i++)
        doParsimUnpacking(buffer, v[i]);
}

// Packing/unpacking an std::list
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::list<T,A>& l)
{
    doParsimPacking(buffer, (int)l.size());
    for (typename std::list<T,A>::const_iterator it = l.begin(); it != l.end(); ++it)
        doParsimPacking(buffer, (T&)*it);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::list<T,A>& l)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        l.push_back(T());
        doParsimUnpacking(buffer, l.back());
    }
}

// Packing/unpacking an std::set
template<typename T, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::set<T,Tr,A>& s)
{
    doParsimPacking(buffer, (int)s.size());
    for (typename std::set<T,Tr,A>::const_iterator it = s.begin(); it != s.end(); ++it)
        doParsimPacking(buffer, *it);
}

template<typename T, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::set<T,Tr,A>& s)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        T x;
        doParsimUnpacking(buffer, x);
        s.insert(x);
    }
}

// Packing/unpacking an std::map
template<typename K, typename V, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::map<K,V,Tr,A>& m)
{
    doParsimPacking(buffer, (int)m.size());
    for (typename std::map<K,V,Tr,A>::const_iterator it = m.begin(); it != m.end(); ++it) {
        doParsimPacking(buffer, it->first);
        doParsimPacking(buffer, it->second);
    }
}

template<typename K, typename V, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::map<K,V,Tr,A>& m)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        K k; V v;
        doParsimUnpacking(buffer, k);
        doParsimUnpacking(buffer, v);
        m[k] = v;
    }
}

// Default pack/unpack function for arrays
template<typename T>
void doParsimArrayPacking(omnetpp::cCommBuffer *b, const T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimPacking(b, t[i]);
}

template<typename T>
void doParsimArrayUnpacking(omnetpp::cCommBuffer *b, T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimUnpacking(b, t[i]);
}

// Default rule to prevent compiler from choosing base class' doParsimPacking() function
template<typename T>
void doParsimPacking(omnetpp::cCommBuffer *, const T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimPacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

template<typename T>
void doParsimUnpacking(omnetpp::cCommBuffer *, T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimUnpacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

}  // namespace omnetpp


template<typename T>
std::string toStringIfPrintable(const T& t) {
    if constexpr (omnetpp::internal::is_printable<T>::value) {
        std::ostringstream os;
        os << t;
        return os.str();
    }
    return omnetpp::cClassDescriptor::UNPRINTABLE;
}

template<typename T>
bool fromStringIfExtractable(T& t, const char *s) {
    if constexpr (omnetpp::internal::is_extractable<T>::value) {
        std::istringstream is(s);
        is >> t;
        return true;
    }
    return false;
}

namespace veins {

Register_Class(EdgeTrustSafetyMessage)

EdgeTrustSafetyMessage::EdgeTrustSafetyMessage(const char *name, short kind) : ::veins::DemoSafetyMessage(name, kind)
{
}

EdgeTrustSafetyMessage::EdgeTrustSafetyMessage(const EdgeTrustSafetyMessage& other) : ::veins::DemoSafetyMessage(other)
{
    copy(other);
}

EdgeTrustSafetyMessage::~EdgeTrustSafetyMessage()
{
}

EdgeTrustSafetyMessage& EdgeTrustSafetyMessage::operator=(const EdgeTrustSafetyMessage& other)
{
    if (this == &other) return *this;
    ::veins::DemoSafetyMessage::operator=(other);
    copy(other);
    return *this;
}

void EdgeTrustSafetyMessage::copy(const EdgeTrustSafetyMessage& other)
{
    this->senderId = other.senderId;
    this->heading = other.heading;
    this->acceleration = other.acceleration;
    this->sequenceNumber = other.sequenceNumber;
    this->retransmissionCount = other.retransmissionCount;
    this->isMalicious_ = other.isMalicious_;
    this->attackType = other.attackType;
}

void EdgeTrustSafetyMessage::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::veins::DemoSafetyMessage::parsimPack(b);
    doParsimPacking(b,this->senderId);
    doParsimPacking(b,this->heading);
    doParsimPacking(b,this->acceleration);
    doParsimPacking(b,this->sequenceNumber);
    doParsimPacking(b,this->retransmissionCount);
    doParsimPacking(b,this->isMalicious_);
    doParsimPacking(b,this->attackType);
}

void EdgeTrustSafetyMessage::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::veins::DemoSafetyMessage::parsimUnpack(b);
    doParsimUnpacking(b,this->senderId);
    doParsimUnpacking(b,this->heading);
    doParsimUnpacking(b,this->acceleration);
    doParsimUnpacking(b,this->sequenceNumber);
    doParsimUnpacking(b,this->retransmissionCount);
    doParsimUnpacking(b,this->isMalicious_);
    doParsimUnpacking(b,this->attackType);
}

int EdgeTrustSafetyMessage::getSenderId() const
{
    return this->senderId;
}

void EdgeTrustSafetyMessage::setSenderId(int senderId)
{
    this->senderId = senderId;
}

double EdgeTrustSafetyMessage::getHeading() const
{
    return this->heading;
}

void EdgeTrustSafetyMessage::setHeading(double heading)
{
    this->heading = heading;
}

double EdgeTrustSafetyMessage::getAcceleration() const
{
    return this->acceleration;
}

void EdgeTrustSafetyMessage::setAcceleration(double acceleration)
{
    this->acceleration = acceleration;
}

int EdgeTrustSafetyMessage::getSequenceNumber() const
{
    return this->sequenceNumber;
}

void EdgeTrustSafetyMessage::setSequenceNumber(int sequenceNumber)
{
    this->sequenceNumber = sequenceNumber;
}

int EdgeTrustSafetyMessage::getRetransmissionCount() const
{
    return this->retransmissionCount;
}

void EdgeTrustSafetyMessage::setRetransmissionCount(int retransmissionCount)
{
    this->retransmissionCount = retransmissionCount;
}

bool EdgeTrustSafetyMessage::isMalicious() const
{
    return this->isMalicious_;
}

void EdgeTrustSafetyMessage::setIsMalicious(bool isMalicious)
{
    this->isMalicious_ = isMalicious;
}

int EdgeTrustSafetyMessage::getAttackType() const
{
    return this->attackType;
}

void EdgeTrustSafetyMessage::setAttackType(int attackType)
{
    this->attackType = attackType;
}

class EdgeTrustSafetyMessageDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_senderId,
        FIELD_heading,
        FIELD_acceleration,
        FIELD_sequenceNumber,
        FIELD_retransmissionCount,
        FIELD_isMalicious,
        FIELD_attackType,
    };
  public:
    EdgeTrustSafetyMessageDescriptor();
    virtual ~EdgeTrustSafetyMessageDescriptor();

    virtual bool doesSupport(omnetpp::cObject *obj) const override;
    virtual const char **getPropertyNames() const override;
    virtual const char *getProperty(const char *propertyName) const override;
    virtual std::string getValueAsString(omnetpp::any_ptr object) const override;
    virtual void setValueAsString(omnetpp::any_ptr object, const char *value) const override;
    virtual int getFieldCount() const override;
    virtual const char *getFieldName(int field) const override;
    virtual int findField(const char *fieldName) const override;
    virtual unsigned int getFieldTypeFlags(int field) const override;
    virtual const char *getFieldTypeString(int field) const override;
    virtual const char **getFieldPropertyNames(int field) const override;
    virtual const char *getFieldProperty(int field, const char *propertyName) const override;
    virtual int getFieldArraySize(omnetpp::any_ptr object, int field) const override;
    virtual void setFieldArraySize(omnetpp::any_ptr object, int field, int size) const override;

    virtual const char *getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const override;
    virtual std::string getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const override;
    virtual omnetpp::cValue getFieldValue(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const override;

    virtual const char *getFieldStructName(int field) const override;
    virtual omnetpp::any_ptr getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const override;
};

Register_ClassDescriptor(EdgeTrustSafetyMessageDescriptor)

EdgeTrustSafetyMessageDescriptor::EdgeTrustSafetyMessageDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(veins::EdgeTrustSafetyMessage)), "veins::DemoSafetyMessage")
{
    propertyNames = nullptr;
}

EdgeTrustSafetyMessageDescriptor::~EdgeTrustSafetyMessageDescriptor()
{
    delete[] propertyNames;
}

bool EdgeTrustSafetyMessageDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<EdgeTrustSafetyMessage *>(obj)!=nullptr;
}

const char **EdgeTrustSafetyMessageDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *EdgeTrustSafetyMessageDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

std::string EdgeTrustSafetyMessageDescriptor::getValueAsString(omnetpp::any_ptr object) const
{
    EdgeTrustSafetyMessage *pp = omnetpp::fromAnyPtr<EdgeTrustSafetyMessage>(object); (void)pp;
    return ((cObject*)pp)->str();
}

void EdgeTrustSafetyMessageDescriptor::setValueAsString(omnetpp::any_ptr object, const char *value) const
{
    EdgeTrustSafetyMessage *pp = omnetpp::fromAnyPtr<EdgeTrustSafetyMessage>(object); (void)pp;
    if (!fromStringIfExtractable(*pp, value))
        cClassDescriptor::setValueAsString(object, value);
}

int EdgeTrustSafetyMessageDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 7+base->getFieldCount() : 7;
}

unsigned int EdgeTrustSafetyMessageDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_senderId
        FD_ISEDITABLE,    // FIELD_heading
        FD_ISEDITABLE,    // FIELD_acceleration
        FD_ISEDITABLE,    // FIELD_sequenceNumber
        FD_ISEDITABLE,    // FIELD_retransmissionCount
        FD_ISEDITABLE,    // FIELD_isMalicious
        FD_ISEDITABLE,    // FIELD_attackType
    };
    return (field >= 0 && field < 7) ? fieldTypeFlags[field] : 0;
}

const char *EdgeTrustSafetyMessageDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "senderId",
        "heading",
        "acceleration",
        "sequenceNumber",
        "retransmissionCount",
        "isMalicious",
        "attackType",
    };
    return (field >= 0 && field < 7) ? fieldNames[field] : nullptr;
}

int EdgeTrustSafetyMessageDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "senderId") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "heading") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "acceleration") == 0) return baseIndex + 2;
    if (strcmp(fieldName, "sequenceNumber") == 0) return baseIndex + 3;
    if (strcmp(fieldName, "retransmissionCount") == 0) return baseIndex + 4;
    if (strcmp(fieldName, "isMalicious") == 0) return baseIndex + 5;
    if (strcmp(fieldName, "attackType") == 0) return baseIndex + 6;
    return base ? base->findField(fieldName) : -1;
}

const char *EdgeTrustSafetyMessageDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "int",    // FIELD_senderId
        "double",    // FIELD_heading
        "double",    // FIELD_acceleration
        "int",    // FIELD_sequenceNumber
        "int",    // FIELD_retransmissionCount
        "bool",    // FIELD_isMalicious
        "int",    // FIELD_attackType
    };
    return (field >= 0 && field < 7) ? fieldTypeStrings[field] : nullptr;
}

const char **EdgeTrustSafetyMessageDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldPropertyNames(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

const char *EdgeTrustSafetyMessageDescriptor::getFieldProperty(int field, const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldProperty(field, propertyName);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

int EdgeTrustSafetyMessageDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    EdgeTrustSafetyMessage *pp = omnetpp::fromAnyPtr<EdgeTrustSafetyMessage>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void EdgeTrustSafetyMessageDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    EdgeTrustSafetyMessage *pp = omnetpp::fromAnyPtr<EdgeTrustSafetyMessage>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'EdgeTrustSafetyMessage'", field);
    }
}

const char *EdgeTrustSafetyMessageDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    EdgeTrustSafetyMessage *pp = omnetpp::fromAnyPtr<EdgeTrustSafetyMessage>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string EdgeTrustSafetyMessageDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    EdgeTrustSafetyMessage *pp = omnetpp::fromAnyPtr<EdgeTrustSafetyMessage>(object); (void)pp;
    switch (field) {
        case FIELD_senderId: return long2string(pp->getSenderId());
        case FIELD_heading: return double2string(pp->getHeading());
        case FIELD_acceleration: return double2string(pp->getAcceleration());
        case FIELD_sequenceNumber: return long2string(pp->getSequenceNumber());
        case FIELD_retransmissionCount: return long2string(pp->getRetransmissionCount());
        case FIELD_isMalicious: return bool2string(pp->isMalicious());
        case FIELD_attackType: return long2string(pp->getAttackType());
        default: return "";
    }
}

void EdgeTrustSafetyMessageDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    EdgeTrustSafetyMessage *pp = omnetpp::fromAnyPtr<EdgeTrustSafetyMessage>(object); (void)pp;
    switch (field) {
        case FIELD_senderId: pp->setSenderId(string2long(value)); break;
        case FIELD_heading: pp->setHeading(string2double(value)); break;
        case FIELD_acceleration: pp->setAcceleration(string2double(value)); break;
        case FIELD_sequenceNumber: pp->setSequenceNumber(string2long(value)); break;
        case FIELD_retransmissionCount: pp->setRetransmissionCount(string2long(value)); break;
        case FIELD_isMalicious: pp->setIsMalicious(string2bool(value)); break;
        case FIELD_attackType: pp->setAttackType(string2long(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'EdgeTrustSafetyMessage'", field);
    }
}

omnetpp::cValue EdgeTrustSafetyMessageDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    EdgeTrustSafetyMessage *pp = omnetpp::fromAnyPtr<EdgeTrustSafetyMessage>(object); (void)pp;
    switch (field) {
        case FIELD_senderId: return pp->getSenderId();
        case FIELD_heading: return pp->getHeading();
        case FIELD_acceleration: return pp->getAcceleration();
        case FIELD_sequenceNumber: return pp->getSequenceNumber();
        case FIELD_retransmissionCount: return pp->getRetransmissionCount();
        case FIELD_isMalicious: return pp->isMalicious();
        case FIELD_attackType: return pp->getAttackType();
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'EdgeTrustSafetyMessage' as cValue -- field index out of range?", field);
    }
}

void EdgeTrustSafetyMessageDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    EdgeTrustSafetyMessage *pp = omnetpp::fromAnyPtr<EdgeTrustSafetyMessage>(object); (void)pp;
    switch (field) {
        case FIELD_senderId: pp->setSenderId(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_heading: pp->setHeading(value.doubleValue()); break;
        case FIELD_acceleration: pp->setAcceleration(value.doubleValue()); break;
        case FIELD_sequenceNumber: pp->setSequenceNumber(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_retransmissionCount: pp->setRetransmissionCount(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_isMalicious: pp->setIsMalicious(value.boolValue()); break;
        case FIELD_attackType: pp->setAttackType(omnetpp::checked_int_cast<int>(value.intValue())); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'EdgeTrustSafetyMessage'", field);
    }
}

const char *EdgeTrustSafetyMessageDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructName(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    };
}

omnetpp::any_ptr EdgeTrustSafetyMessageDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    EdgeTrustSafetyMessage *pp = omnetpp::fromAnyPtr<EdgeTrustSafetyMessage>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void EdgeTrustSafetyMessageDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    EdgeTrustSafetyMessage *pp = omnetpp::fromAnyPtr<EdgeTrustSafetyMessage>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'EdgeTrustSafetyMessage'", field);
    }
}

}  // namespace veins

namespace omnetpp {

}  // namespace omnetpp

