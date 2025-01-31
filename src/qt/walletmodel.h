// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// File contains modifications by: The Gulden developers
// All modifications:
// Copyright (c) 2016-2018 The Gulden developers
// Authored by: Malcolm MacLeod (mmacleod@webmail.co.za)
// Distributed under the GULDEN software license, see the accompanying
// file COPYING

#ifndef GULDEN_QT_WALLETMODEL_H
#define GULDEN_QT_WALLETMODEL_H

#include "paymentrequestplus.h"
#include "walletmodeltransaction.h"

#include "support/allocators/secure.h"
#include "account.h"

#include <map>
#include <vector>

#include <QObject>
#include <QRegularExpression>

#define BOOST_UUID_RANDOM_PROVIDER_FORCE_POSIX
#include <boost/uuid/uuid.hpp>
#include "wallet/wallet.h"

class AddressTableModel;
class AccountTableModel;
class OptionsModel;
class QStyle;
class RecentRequestsTableModel;
class TransactionTableModel;
class WalletModelTransaction;

class CCoinControl;
class CKeyID;
class COutPoint;
class COutput;
class CPubKey;
class CWallet;
class uint256;
class CAccount;

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

class SendCoinsRecipient
{
public:
    explicit SendCoinsRecipient() : amount(0), fSubtractFeeFromAmount(false), nVersion(SendCoinsRecipient::CURRENT_VERSION), destinationPoW2Witness(CKeyID(), CKeyID()) { }
    explicit SendCoinsRecipient(const QString &addr, const QString &_label, const CAmount& _amount, const QString &_message):
        address(addr), label(_label), amount(_amount), message(_message), fSubtractFeeFromAmount(false), paymentType(PaymentType::NormalPayment), nVersion(SendCoinsRecipient::CURRENT_VERSION), destinationPoW2Witness(CKeyID(), CKeyID()) {}

    SendCoinsRecipient(const SendCoinsRecipient& copy)
    {
        address = copy.address;
        label = copy.label;
        amount = copy.amount;
        message = copy.message;
        paymentRequest = copy.paymentRequest;
        authenticatedMerchant = copy.authenticatedMerchant;
        fSubtractFeeFromAmount = copy.fSubtractFeeFromAmount;
        addToAddressBook = copy.addToAddressBook;
        paymentType = copy.paymentType;
        forexPaymentType = copy.forexPaymentType;
        forexAddress = copy.forexAddress;
        forexDescription = copy.forexDescription;
        forexAmount = copy.forexAmount;
        forexFailCode = copy.forexFailCode;
        expiry = copy.expiry;
        nVersion = copy.nVersion;
        witnessForAccount = copy.witnessForAccount;
        destinationPoW2Witness = copy.destinationPoW2Witness;
    }
    // If from an unauthenticated payment request, this is used for storing
    // the addresses, e.g. address-A<br />address-B<br />address-C.
    // Info: As we don't need to process addresses in here when using
    // payment requests, we can abuse it for displaying an address list.
    // Todo: This is a hack, should be replaced with a cleaner solution!
    QString address;
    QString label;
    CAmount amount;
    // If from a payment request, this is used for storing the memo
    QString message;

    // If from a payment request, paymentRequest.IsInitialized() will be true
    PaymentRequestPlus paymentRequest;
    // Empty if no authentication or invalid signature/cert/etc.
    QString authenticatedMerchant;

    enum PaymentType
    {
        NormalPayment,
        IBANPayment,
        BitcoinPayment,
        InvalidPayment
    };
    bool fSubtractFeeFromAmount; // memory only
    bool addToAddressBook; //memory only
    PaymentType paymentType; //memory only
    PaymentType forexPaymentType; //memory only
    QString forexAddress; //memory only
    QString forexDescription; //memory only
    CAmount forexAmount; //memory only
    std::string forexFailCode; //memory only
    int64_t expiry; //memory only


    static const int CURRENT_VERSION = 1;
    int nVersion;

    //! Witness for account should only be set when "destinationPoW2Witness" is funding a "never used before"
    //! witness key ID (e.g. when funding a witness account for the first time).
    CAccount* witnessForAccount = nullptr;
    CPoW2WitnessDestination destinationPoW2Witness;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        std::string sAddress = address.toStdString();
        std::string sLabel = label.toStdString();
        std::string sMessage = message.toStdString();
        std::string sPaymentRequest;
        if (!ser_action.ForRead() && paymentRequest.IsInitialized())
            paymentRequest.SerializeToString(&sPaymentRequest);
        std::string sAuthenticatedMerchant = authenticatedMerchant.toStdString();

        //fixme: (2.1) Is it necessary to serialise the pow2 stuff here? Looks like its only used for merchant stuff which should never be happening with witnesses...
        READWRITE(this->nVersion);
        READWRITE(sAddress);
        READWRITE(sLabel);
        READWRITE(amount);
        READWRITE(sMessage);
        READWRITE(sPaymentRequest);
        READWRITE(sAuthenticatedMerchant);

        try
        {
            READWRITE(destinationPoW2Witness);
        }
        catch(...)
        {
        }

        if (ser_action.ForRead())
        {
            address = QString::fromStdString(sAddress);
            label = QString::fromStdString(sLabel);
            message = QString::fromStdString(sMessage);
            if (!sPaymentRequest.empty())
                paymentRequest.parse(QByteArray::fromRawData(sPaymentRequest.data(), sPaymentRequest.size()));
            authenticatedMerchant = QString::fromStdString(sAuthenticatedMerchant);
        }
    }
};

/** Interface to Gulden wallet from Qt view code. */
class WalletModel : public QObject
{
    Q_OBJECT

public:
    explicit WalletModel(const QStyle *platformStyle, CWallet *wallet, OptionsModel *optionsModel, QObject *parent = 0);
    virtual ~WalletModel();

    enum StatusCode // Returned by sendCoins
    {
        OK,
        InvalidAmount,
        InvalidAddress,
        AmountExceedsBalance,
        AmountWithFeeExceedsBalance,
        DuplicateAddress,
        TransactionCreationFailed, // Error returned when wallet is still locked
        TransactionCommitFailed,
        AbsurdFee,
        PaymentRequestExpired,
        PoW2NotActive,
        ForexFailed
    };

    enum EncryptionStatus
    {
        Unencrypted,  // !wallet->IsCrypted()
        Locked,       // wallet->IsCrypted() && wallet->IsLocked()
        Unlocked      // wallet->IsCrypted() && !wallet->IsLocked()
    };

    OptionsModel *getOptionsModel();
    AddressTableModel *getAddressTableModel();
    AccountTableModel *getAccountTableModel();
    TransactionTableModel *getTransactionTableModel();
    RecentRequestsTableModel *getRecentRequestsTableModel();

    CAmount getBalance(CAccount* forAccount=NULL, const CCoinControl *coinControl = NULL) const;
    CAmount getUnconfirmedBalance(CAccount* forAccount=NULL) const;
    CAmount getImmatureBalance() const;
    bool haveWatchOnly() const;
    CAmount getWatchBalance() const;
    CAmount getWatchUnconfirmedBalance() const;
    CAmount getWatchImmatureBalance() const;
    WalletBalances getBalances() const;
    EncryptionStatus getEncryptionStatus() const;

    // Check address for validity
    bool validateAddress(const QString &address);
    bool validateAddressBitcoin(const QString &address);
    bool validateAddressIBAN(const QString &address);

    // Return status record for SendCoins, contains error id + information
    struct SendCoinsReturn
    {
        SendCoinsReturn(StatusCode _status = OK, QString _reasonCommitFailed = "")
            : status(_status),
              reasonCommitFailed(_reasonCommitFailed)
        {
        }
        StatusCode status;
        QString reasonCommitFailed;
    };

    // prepare transaction for getting txfee before sending coins
    SendCoinsReturn prepareTransaction(CAccount* forAccount, WalletModelTransaction &transaction, const CCoinControl *coinControl = NULL);

    // Send coins to a list of recipients
    SendCoinsReturn sendCoins(WalletModelTransaction &transaction);

    // Wallet encryption
    bool setWalletEncrypted(bool encrypted, const SecureString &passphrase);
    // Passphrase only needed when unlocking
    bool setWalletLocked(bool locked, const SecureString &passPhrase=SecureString());
    bool changePassphrase(const SecureString &oldPass, const SecureString &newPass);
    // Wallet backup
    bool backupWallet(const QString &filename);

    // RAI object for unlocking wallet, returned by requestUnlock()
    class UnlockContext
    {
    public:
        UnlockContext(WalletModel *wallet, bool valid, bool relock);
        ~UnlockContext();

        bool isValid() const { return valid; }

        // Copy operator and constructor transfer the context
        UnlockContext(const UnlockContext& obj) { CopyFrom(obj); }
        UnlockContext& operator=(const UnlockContext& rhs) { CopyFrom(rhs); return *this; }
    private:
        WalletModel *wallet;
        bool valid;
        mutable bool relock; // mutable, as it can be set to false by copying

        void CopyFrom(const UnlockContext& rhs);
    };

    UnlockContext requestUnlock();

    bool getPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const;
    bool havePrivKey(const CKeyID &address) const;
    bool getPrivKey(const CKeyID &address, CKey& vchPrivKeyOut) const;
    void getOutputs(const std::vector<COutPoint>& vOutpoints, std::vector<COutput>& vOutputs);
    bool isSpent(const COutPoint& outpoint) const;
    void listCoins(CAccount* forAccount, std::map<QString, std::vector<COutput> >& mapCoins) const;

    bool isLockedCoin(uint256 hash, unsigned int n) const;
    void lockCoin(COutPoint& output);
    void unlockCoin(COutPoint& output);
    void listLockedCoins(std::vector<COutPoint>& vOutpts);

    void loadReceiveRequests(std::vector<std::string>& vReceiveRequests);
    bool saveReceiveRequest(const std::string &sAddress, const int64_t nId, const std::string &sRequest);

    bool transactionCanBeAbandoned(uint256 hash) const;
    bool abandonTransaction(uint256 hash) const;

    bool transactionCanBeBumped(uint256 hash) const;
    bool bumpFee(uint256 hash);

    static bool isWalletEnabled();

    bool hdEnabled() const;

    int getDefaultConfirmTarget() const;

    bool getDefaultWalletRbf() const;

    void setActiveAccount( CAccount* account );
    CAccount* getActiveAccount();
    QString getAccountLabel(const boost::uuids::uuid& uuid);

    void unsubscribeFromCoreSignals();

private:
    CWallet *wallet;
    bool fHaveWatchOnly;
    bool fForceCheckBalanceChanged;

    // Wallet has an options model for wallet-specific options
    // (transaction fee, for example)
    OptionsModel *optionsModel;

    AddressTableModel *addressTableModel;
    AccountTableModel *accountTableModel;
    TransactionTableModel *transactionTableModel;
    RecentRequestsTableModel *recentRequestsTableModel;

    // Cache some values to be able to detect changes
    mutable WalletBalances cachedBalances;
    CAmount cachedWatchOnlyBalance = -1;
    CAmount cachedWatchUnconfBalance = -1;
    CAmount cachedWatchImmatureBalance = -1;
    EncryptionStatus cachedEncryptionStatus;
    int cachedNumBlocks;

    QTimer *pollTimer;

    void subscribeToCoreSignals();
    void checkBalanceChanged();

    QRegularExpression patternMatcherIBAN;


Q_SIGNALS:
    // Signal that balance in wallet changed
    void balanceChanged(const WalletBalances& balances, const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance);

    // Encryption status of wallet changed
    void encryptionStatusChanged(int status);

    // Signal emitted when wallet needs to be unlocked
    // It is valid behaviour for listeners to keep the wallet locked after this signal;
    // this means that the unlocking failed or was cancelled.
    void requireUnlock();

    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);

    // Coins sent: from wallet, to recipient, in (serialized) transaction:
    void coinsSent(CWallet* wallet, SendCoinsRecipient recipient, QByteArray transaction);

    // Show progress dialog e.g. for rescan
    void showProgress(const QString &title, int nProgress);

    // Watch-only address added
    void notifyWatchonlyChanged(bool fHaveWatchonly);

    void activeAccountChanged(CAccount* account);
    void accountNameChanged(CAccount* account);
    void accountWarningChanged(CAccount* account);
    void accountAdded(CAccount* account);
    void accountDeleted(CAccount* account);

public Q_SLOTS:
    /* Wallet status might have changed */
    void updateStatus();
    /* New transaction, or transaction changed status */
    void updateTransaction();
    /* New, updated or removed address book entry */
    void updateAddressBook(const QString &address, const QString &label, bool isMine, const QString &purpose, int status);
    /* Watch-only added */
    void updateWatchOnlyFlag(bool fHaveWatchonly);
    /* Current, immature or unconfirmed balance might have changed - emit 'balanceChanged' if so */
    void pollBalanceChanged();
};

#endif // GULDEN_QT_WALLETMODEL_H
