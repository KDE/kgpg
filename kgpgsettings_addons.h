public:
   static
   QString defaultKey()
   {
     if (self()->mDefaultKey.isEmpty())
     {
        self()->mDefaultKey = KgpgInterface::getGpgSetting("default-key", gpgConfigPath());
        if (!self()->mDefaultKey.isEmpty())
           self()->mDefaultKey.prepend("0x");
     }
     return self()->mDefaultKey;
   }
   
   static
   void setDefaultKey(const QString &_defaultKey)
   {
     self()->mDefaultKey = _defaultKey;
     KgpgInterface::setGpgSetting("default-key",_defaultKey.right(8),gpgConfigPath());
     if (!_defaultKey.startsWith("0x"))
        self()->mDefaultKey.prepend("0x");
   }
   
private:
   QString mDefaultKey;
