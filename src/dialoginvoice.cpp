/**
  This file is a part of mcercle
  Copyright (C) 2010-2013 Cyril FRAUSTI

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.

*/

#include "dialoginvoice.h"
#include "ui_dialoginvoice.h"

#include "printc.h"
#include "table.h"
#include "mctextedit.h"
#include "dialogtax.h"

#include <QMessageBox>
#include <QMenu>
#include <QDebug>
#include <QSplitter>
#include <QUrl>
#include <QFileDialog>
#include <QImageReader>
#include <tgmath.h>


DialogInvoice::DialogInvoice(QLocale &lang, database *pdata, unsigned char type, unsigned char state, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DialogInvoice)
{
	ui->setupUi(this);
	setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint & Qt::WindowMaximizeButtonHint);
	// la fenetre est maximisee par defaut
	setMinimumSize(QSize(640, 580));
	setWindowState(Qt::WindowMaximized);

	m_data			= pdata;
	m_customer		= pdata -> m_customer;
	m_proposal		= pdata -> m_customer -> m_proposal;
	m_invoice		= pdata -> m_customer -> m_invoice;
	m_service		= pdata -> m_customer -> m_service;
	m_serviceComm	= pdata -> m_customer -> m_serviceComm;
	m_product		= pdata -> m_product;
	m_isTax			= pdata -> getIsTax();
	m_lang			= lang;

	//Information bdd
	database::Informations inf;
	m_data->getInfo( inf );
	m_manageStock = inf.manageStock;

	m_DialogType	= type;
	m_DialogState	= state;
	//Configuration de l'UI
	setUI();

	//Event
	connect(ui->lineEdit_code, SIGNAL(textChanged(const QString &)), this, SLOT(checkConditions()));
	connect(ui->lineEdit_description, SIGNAL(textChanged(const QString &)), this, SLOT(checkConditions()));
}

DialogInvoice::~DialogInvoice() {
	delete ui;
}

/**
	 Test les conditions pour activer le bouton Ajouter/modifier
  */
void DialogInvoice::checkConditions() {
	if((!ui->lineEdit_description->text().isEmpty())
		&& (!ui->lineEdit_code->text().isEmpty())
		)
		ui->pushButton_ok->setEnabled(true);
	else
		ui->pushButton_ok->setEnabled(false);
}

/**
	Renseigne le titre
	@param titre de la fenetre
  */
void DialogInvoice::setTitle(QString val){
	ui->labelTitle->setText(val);
}

/**
 * @brief DialogInvoice::setUI
 */
void DialogInvoice::setUI() {
	QStringList plist;
	ui -> pushButton_print -> setEnabled( false );
	ui -> comboBox_State->clear();
	ui -> comboBox_TYPE_PAYMENT->clear();
	ui -> tableWidget->clear();

	if(m_DialogType == PROPOSAL_TYPE){
		this->setWindowTitle( tr("Devis") );
		plist << "" << tr("Espece") << tr("Cheque") << tr("CB") << tr("TIP") << tr("Virement") << tr("Prelevement") << tr("Autre");
		ui->comboBox_TYPE_PAYMENT->addItems(plist);
		ui->comboBox_State->addItem(m_proposal->getIconState( MCERCLE::PROPOSAL_WRITING ),
								m_proposal->getTextState( MCERCLE::PROPOSAL_WRITING ));
		ui->comboBox_State->addItem(m_proposal->getIconState( MCERCLE::PROPOSAL_PROPOSED ),
								m_proposal->getTextState( MCERCLE::PROPOSAL_PROPOSED ));
		ui->comboBox_State->addItem(m_proposal->getIconState( MCERCLE::PROPOSAL_VALIDATED ),
								m_proposal->getTextState( MCERCLE::PROPOSAL_VALIDATED ));

		///TODO: asupprimer la date de livraison
		ui->label_delivery->setText(tr("Date de livraison :"));
		ui->label_delivery->setVisible(false);
		ui->dateEdit_delivery->setVisible(false);

		ui->label_link->setText(QLatin1String("Facture associ�e : "));
		ui->label_partpayment->setVisible(false);
		ui->pushButton_partInvoice->setVisible(false);
		ui->pushButton_creditInvoice->setVisible(false);

	}
	else{
		this->setWindowTitle( tr("Facture") );
		plist << "" << "Espece" << "Cheque" << "CB" << "TIP" << "Virement" << "Prelevement" << "Autre";
		ui->comboBox_TYPE_PAYMENT->addItems(plist);

		ui->comboBox_State->addItem(m_invoice->getIconState( MCERCLE::INV_UNPAID ),
								m_invoice->getTextState( MCERCLE::INV_UNPAID ));
		ui->comboBox_State->addItem(m_invoice->getIconState( MCERCLE::INV_PAID ),
								m_invoice->getTextState( MCERCLE::INV_PAID ));
		ui->comboBox_State->addItem(m_invoice->getIconState( MCERCLE::INV_OVERDUE ),
								m_invoice->getTextState( MCERCLE::INV_OVERDUE ));
		ui->comboBox_State->addItem(m_invoice->getIconState( MCERCLE::INV_CANCEL ),
								m_invoice->getTextState( MCERCLE::INV_CANCEL ));
		
		ui->label_delivery->setText(QLatin1String("Date Ech�ance :"));
		ui->label_link->setText(QLatin1String("Devis associ�e : "));
		ui->label_datevalid->setText(QLatin1String("Date R�glement :"));
		ui->pushButton_createInv->setVisible(false);
	}
	ui->pushButton_ok->setEnabled(false);
	// Si c'est une �dition
	if(m_DialogState == EDIT){
		ui -> pushButton_print -> setEnabled( true );
		ui->pushButton_ok->setText(tr("Modifier"));
		ui->pushButton_ok->setIcon(QIcon(":/app/Edit"));
		loadValues();
		if(m_DialogType == PROPOSAL_TYPE) {
			listProposalDetailsToTable("", "");
		}
		else{
			listInvoiceDetailsToTable("", "");
			//Test si facture acompte ou avoir
			if( m_invoice->getType() != MCERCLE::TYPE_INV) {
				ui->label_link ->setText( QLatin1String("Sur Facture :") );
				ui->lineEdit_linkCode->setText( m_invoice->getCodeInvoice_Ref() );
				ui->label_partpayment->setVisible(false);
				ui->pushButton_partInvoice->setEnabled(false);
				ui->pushButton_creditInvoice->setEnabled(false);
			}
		}
	}
	// Sinon c'est une creation !
	else{
		//DEVIS
		if(m_DialogType == PROPOSAL_TYPE)
			m_proposal->setIdCustomer( m_customer->getId() );
		//FActure
		else{
			m_invoice -> setDefault(); // raz la class
			m_invoice -> setIdCustomer( m_customer->getId() );
		}
		
		ui->dateEdit_DATE->setDateTime( QDateTime::currentDateTime());
		ui->dateEdit_delivery->setDateTime( QDateTime::currentDateTime());

		if(m_DialogType == PROPOSAL_TYPE)
			ui->dateEdit_valid->setDateTime( QDateTime::currentDateTime().addMonths(1)); /* 1mois pour la duree du devis*/
		else
			ui->dateEdit_valid->setDateTime( QDateTime::currentDateTime());
		
		ui->pushButton_ok->setText(tr("Ajouter"));
		ui->pushButton_ok->setIcon(QIcon(":/app/insert"));
		ui->lineEdit_code->setText(tr("Automatique"));
		ui->pushButton_createInv->setVisible(false);
	}
	//lister les services
	listServiceToTable();
	//Cache le layout select base de donnee
	ui->groupBox_select->setVisible( false );
	ui->toolButton_add->setVisible( false );

	//Selectionne la tab 0
	ui->tabWidget_select->setCurrentIndex(0);

	//Products
	m_productView = new productView( m_data, m_lang, productView::INVOICE_VIEW );
	//Mis en layout
	ui->productLayout->addWidget( m_productView );

	//Applique un menu au bouton ajout libre
	QMenu *menu = new QMenu(tr("Type"));
	QAction *sevAct = menu -> addAction(QIcon(":/app/services"), tr("Service"));/*QKeySequence(Qt::CTRL + Qt::Key_P);*/
	QAction *prodAct = menu -> addAction(QIcon(":/app/products"), tr("Produit"));
	ui->toolButton_addFreeline->setMenu( menu );
	connect(sevAct, SIGNAL(triggered()), this, SLOT(addFreeline_Service()));
	connect(prodAct, SIGNAL(triggered()), this, SLOT(addFreeline_Product()));
	
	//Applique un menu au bouton acompte
	QMenu *menuAcompte = new QMenu(tr("Acompte"));
	QAction *sevAcompte = menuAcompte -> addAction(QIcon(":/app/services"), tr("Service"));/*QKeySequence(Qt::CTRL + Qt::Key_P);*/
	QAction *prodAcompte = menuAcompte -> addAction(QIcon(":/app/products"), tr("Produit"));
	ui->pushButton_partInvoice->setMenu( menuAcompte );
	connect(sevAcompte, SIGNAL(triggered()), this, SLOT(create_service_partInvoice()));
	connect(prodAcompte, SIGNAL(triggered()), this, SLOT(create_product_partInvoice()));

	//Test les conditions
	checkConditions();
}

/**
  Renseigne les informations
  */
void DialogInvoice::loadValues(){
	if(m_DialogType == PROPOSAL_TYPE){
		ui->lineEdit_code->setText( m_proposal->getCode() );
		ui->lineEdit_linkCode->setText( m_proposal->getInvoiceCode() );
		ui->lineEdit_description->setText( m_proposal->getDescription() );
		ui->dateTimeEdit->setDateTime( m_proposal->getCreationDate() );
		ui->dateEdit_DATE->setDate( m_proposal->getUserDate() );
		ui->dateEdit_delivery->setDate( m_proposal->getDeliveryDate() );
		ui->dateEdit_valid->setDate(m_proposal->getValidDate());

		ui->comboBox_State->setCurrentIndex( m_proposal->getState());
		//Si deja signee et pas associee deja a une facture on autorise la creation dune facture
		if( (m_proposal->getState() ==  MCERCLE::PROPOSAL_VALIDATED) && (m_proposal->getInvoiceCode().isEmpty()) )
				ui->pushButton_createInv->setEnabled(true);
		else	ui->pushButton_createInv->setEnabled(false);

		QString typeP = m_proposal->getTypePayment();
		if(typeP.isEmpty() || typeP.isNull()) ui->comboBox_TYPE_PAYMENT->setCurrentIndex(0);
		if(typeP == MCERCLE::CASH) ui->comboBox_TYPE_PAYMENT->setCurrentIndex(1);
		if(typeP == MCERCLE::CHECK) ui->comboBox_TYPE_PAYMENT->setCurrentIndex(2);
		if(typeP == MCERCLE::CREDIT_CARD) ui->comboBox_TYPE_PAYMENT->setCurrentIndex(3);
		if(typeP == MCERCLE::INTERBANK) ui->comboBox_TYPE_PAYMENT->setCurrentIndex(4);
		if(typeP == MCERCLE::TRANSFER) ui->comboBox_TYPE_PAYMENT->setCurrentIndex(5);
		if(typeP == MCERCLE::DEBIT) ui->comboBox_TYPE_PAYMENT->setCurrentIndex(6);
		if(typeP == MCERCLE::OTHER) ui->comboBox_TYPE_PAYMENT->setCurrentIndex(7);
	}
	else{
		ui->lineEdit_code->setText( m_invoice->getCode() );
		ui->lineEdit_linkCode->setText( m_invoice->getProposalCode() );
		ui->lineEdit_description->setText( m_invoice->getDescription() );
		ui->dateTimeEdit->setDateTime( m_invoice->getCreationDate() );
		ui->dateEdit_DATE->setDate( m_invoice->getUserDate() );
		ui->dateEdit_delivery->setDate( m_invoice->getLimitPayment() ); /* Limit de paiement*/
		if(m_isTax) {
		ui->label_partpayment->setText(
					tr("Total accompte(s):")+
					tr("\nHT : ")+m_lang.toString(m_invoice->getPartPayment(),'f',2) +
					tr("\nTTC: ")+m_lang.toString(m_invoice->getPartPaymentTax(),'f',2) );
		}
		else{
			ui->label_partpayment->setText(
					tr("Total accompte(s): ")+m_lang.toString(m_invoice->calcul_partPayment(m_invoice->getId()),'f',2) );
		}
		ui->dateEdit_valid->setDate( m_invoice->getPaymentDate() ); /* Date de paiement*/

		ui->comboBox_State->setCurrentIndex( m_invoice->getState());
		QString typeP = m_invoice->getTypePayment();
		//if(typeP.isEmpty() || typeP.isNull()) ui->comboBox_TYPE_PAYMENT->setCurrentIndex(0);
		if(typeP == MCERCLE::CASH) ui->comboBox_TYPE_PAYMENT->setCurrentIndex(1);
		if(typeP == MCERCLE::CHECK) ui->comboBox_TYPE_PAYMENT->setCurrentIndex(2);
		if(typeP == MCERCLE::CREDIT_CARD) ui->comboBox_TYPE_PAYMENT->setCurrentIndex(3);
		if(typeP == MCERCLE::INTERBANK) ui->comboBox_TYPE_PAYMENT->setCurrentIndex(4);
		if(typeP == MCERCLE::TRANSFER) ui->comboBox_TYPE_PAYMENT->setCurrentIndex(5);
		if(typeP == MCERCLE::DEBIT) ui->comboBox_TYPE_PAYMENT->setCurrentIndex(6);
		if(typeP == MCERCLE::OTHER) ui->comboBox_TYPE_PAYMENT->setCurrentIndex(7);
	}
}


/**
	Affiche les articles dans le tableau
	@param filter, filtre a appliquer
	@param field, champ ou appliquer le filtre
*/
void DialogInvoice::listProposalDetailsToTable(QString filter, QString field)
{
	proposal::ProposalListItems ilist;

	//Clear les items, attention tjs utiliser la fonction clear()
	ui->tableWidget->clear();
	for (int i=ui->tableWidget->rowCount()-1; i >= 0; --i)
		ui->tableWidget->removeRow(i);
	for (int i=ui->tableWidget->columnCount()-1; i>=0; --i)
		ui->tableWidget->removeColumn(i);

	ui->tableWidget->setSortingEnabled(false);
	//Style de la table de proposition
	ui->tableWidget->setColumnCount(COL_COUNT);
    ui->tableWidget->setColumnWidth(COL_NAME,375);

#ifdef QT_NO_DEBUG
	ui->tableWidget->setColumnHidden(COL_ID , true); //cache la colonne ID ou DEBUG
	ui->tableWidget->setColumnHidden(COL_ID_PRODUCT , true); //cache la colonne ID ou DEBUG
	ui->tableWidget->setColumnHidden(COL_ORDER , true); //cache la order ou DEBUG
	ui->tableWidget->setColumnHidden(COL_TYPE , true); //cache la order ou DEBUG
#endif

	QStringList titles;
	titles  << tr("Id") << tr("Id Produit") << tr("Type") << tr("Ordre") << tr("Nom") << tr("Tva") << tr("Remise(%)") << tr("Prix") << QLatin1String("Quantit�");
	if(!m_isTax){
			titles << tr("Total");
			ui->tableWidget->setColumnHidden(COL_TAX , true); //cache la colonne TVA
	}
	else{
		titles << tr("Total HT");
	}
	ui->tableWidget->setHorizontalHeaderLabels( titles );

	//Recuperation des donnees presentent dans la bdd
	m_proposal->getProposalItemsList(ilist, "ITEM_ORDER", filter, field);
	// list all
	for(int i=0; i<ilist.name.count(); i++){
		ItemOfTable *item_ID           = new ItemOfTable();
		ItemOfTable *item_ID_PRODUCT   = new ItemOfTable();
		ItemOfTable *item_TYPE         = new ItemOfTable();
		ItemOfTable *item_ORDER        = new ItemOfTable();
		ItemOfTable *item_Name         = new ItemOfTable();
		ItemOfTable *item_TAX          = new ItemOfTable();
		ItemOfTable *item_DISCOUNT     = new ItemOfTable();
		ItemOfTable *item_PRICE        = new ItemOfTable();
		ItemOfTable *item_QUANTITY     = new ItemOfTable();


		item_ID->setData(Qt::DisplayRole, ilist.id.at(i));
		item_ID->setFlags(item_ID->flags() & (~Qt::ItemIsEditable)); // Ne pas rendre editable
		item_ID_PRODUCT->setData(Qt::DisplayRole, ilist.idProduct.at(i));
		item_ID_PRODUCT->setFlags(item_ID_PRODUCT->flags() & (~Qt::ItemIsEditable)); // Ne pas rendre editable
		item_TYPE->setData(Qt::DisplayRole, ilist.type.at(i));
		item_TYPE->setFlags(item_ORDER->flags() & (~Qt::ItemIsEditable)); // Ne pas rendre editable
		item_ORDER->setData(Qt::DisplayRole, ilist.order.at(i));
		item_ORDER->setFlags(item_ORDER->flags() & (~Qt::ItemIsEditable)); // Ne pas rendre editable
		
		QTextEdit *edit_name = new QTextEdit();
		edit_name ->setText( ilist.name.at(i) );
		// Ne pas rendre editable si c est un produit! + Ajout d'image
		// Car il il y a une liaison pour la gestion du stock
		if(ilist.idProduct.at(i) > 0){
			edit_name -> setReadOnly(true);
			item_Name -> setIcon( QIcon(":/app/database") );
		}
		else if(ilist.type.at(i) == MCERCLE::PRODUCT) {
			item_Name -> setIcon( QIcon(":/app/products") );
		}
		else if(ilist.type.at(i) == MCERCLE::SERVICE) {
			item_Name -> setIcon( QIcon(":/app/services") );
		}
		item_TAX->setData(Qt::DisplayRole, ilist.tax.at(i));
		item_DISCOUNT->setData(Qt::DisplayRole, ilist.discount.at(i));
		item_PRICE->setData(Qt::DisplayRole, ilist.price.at(i));
		item_QUANTITY->setData(Qt::DisplayRole, ilist.quantity.at(i));

		//definir le tableau
		ui->tableWidget->setRowCount(i+1);

		//remplir les champs
		ui->tableWidget->setItem(i, COL_ID, item_ID);
		ui->tableWidget->setItem(i, COL_ID_PRODUCT, item_ID_PRODUCT);
		ui->tableWidget->setItem(i, COL_TYPE, item_TYPE);
		ui->tableWidget->setItem(i, COL_ORDER, item_ORDER);
		ui->tableWidget->setItem(i, COL_NAME, item_Name);
		ui->tableWidget->setCellWidget(i, COL_NAME, edit_name);
		ui->tableWidget->setItem(i, COL_TAX, item_TAX);
		ui->tableWidget->setItem(i, COL_DISCOUNT, item_DISCOUNT);
		ui->tableWidget->setItem(i, COL_PRICE, item_PRICE);
		ui->tableWidget->setItem(i, COL_QUANTITY, item_QUANTITY);
		
		// Applique la hauteur de la ligne en fonction des new lines
		ui->tableWidget->setRowHeight(i, (ilist.name.at(i).count("\n")+1) * 35); 

	}
	ui->tableWidget->setSortingEnabled(true);
	ui->tableWidget->sortItems(COL_ORDER, Qt::AscendingOrder);
	ui->tableWidget->selectRow(0);
}

/**
	Affiche les articles de la facture du client
	@param filter, filtre a appliquer
	@param field, champ ou appliquer le filtre
*/
void DialogInvoice::listInvoiceDetailsToTable(QString filter, QString field) {
	m_ilist.id.clear();
	m_ilist.idProduct.clear();
	m_ilist.type.clear();
	m_ilist.discount.clear();
	m_ilist.name.clear();
	m_ilist.price.clear();
	m_ilist.quantity.clear();
	m_ilist.tax.clear();
	m_ilist.order.clear();

	//Clear les items, attention tjs utiliser la fonction clear()
	ui->tableWidget->clear();
	for (int i=ui->tableWidget->rowCount()-1; i >= 0; --i)
		ui->tableWidget->removeRow(i);
	for (int i=ui->tableWidget->columnCount()-1; i>=0; --i)
		ui->tableWidget->removeColumn(i);

	ui->tableWidget->setSortingEnabled(false);
	//Style de la table de facture
	ui->tableWidget->setColumnCount(COL_COUNT);
    ui->tableWidget->setColumnWidth(COL_NAME,375);
#ifdef QT_NO_DEBUG
	ui->tableWidget->setColumnHidden(COL_ID , true); //cache la colonne ID ou DEBUG
	ui->tableWidget->setColumnHidden(COL_ID_PRODUCT , true); //cache la colonne ID ou DEBUG
	ui->tableWidget->setColumnHidden(COL_ORDER , true); //cache la order ou DEBUG
	ui->tableWidget->setColumnHidden(COL_TYPE , true); //cache la order ou DEBUG
#endif

	QStringList titles;
	titles  << tr("Id") << tr("Id Produit") << tr("Type") << tr("Ordre") << tr("Nom") << tr("Tva") << tr("Remise(%)") << tr("Prix") << QLatin1String("Quantit�");
	if(!m_isTax){
			titles << tr("Total");
			ui->tableWidget->setColumnHidden(COL_TAX , true); //cache la colonne TVA
	}
	else	titles << tr("Total HT");
	ui->tableWidget->setHorizontalHeaderLabels( titles );

	//Recuperation des donnees presentent dans la bdd
	m_invoice->getInvoiceItemsList(m_ilist, "ITEM_ORDER", filter, field);
	// list all
	for(int i=0; i<m_ilist.name.count(); i++){
		ItemOfTable *item_ID           = new ItemOfTable();
		ItemOfTable *item_ID_PRODUCT   = new ItemOfTable();
		ItemOfTable *item_TYPE         = new ItemOfTable();
		ItemOfTable *item_ORDER        = new ItemOfTable();
		ItemOfTable *item_Name         = new ItemOfTable();
		ItemOfTable *item_TAX          = new ItemOfTable();
		ItemOfTable *item_DISCOUNT     = new ItemOfTable();
		ItemOfTable *item_PRICE        = new ItemOfTable();
		ItemOfTable *item_QUANTITY     = new ItemOfTable();

		item_ID->setData(Qt::DisplayRole, m_ilist.id.at(i));
		item_ID->setFlags(item_ID->flags() & (~Qt::ItemIsEditable)); // Ne pas rendre editable
		item_ID_PRODUCT->setData(Qt::DisplayRole, m_ilist.idProduct.at(i));
		item_ID_PRODUCT->setFlags(item_ID_PRODUCT->flags() & (~Qt::ItemIsEditable)); // Ne pas rendre editable
		item_TYPE->setData(Qt::DisplayRole, m_ilist.type.at(i));
		item_TYPE->setFlags(item_ORDER->flags() & (~Qt::ItemIsEditable)); // Ne pas rendre editable
		item_ORDER->setData(Qt::DisplayRole, m_ilist.order.at(i));
		item_ORDER->setFlags(item_ORDER->flags() & (~Qt::ItemIsEditable)); // Ne pas rendre editable
		
		MCTextEdit *edit_name = new MCTextEdit();
		edit_name ->setText( m_ilist.name.at(i) );

		// Ne pas rendre editable si c est un produit! + Ajout d'image
		// Car il il y a une liaison pour la gestion du stock
		if(m_ilist.idProduct.at(i) > 0){
			edit_name -> setReadOnly(true);
			item_Name -> setIcon( QIcon(":/app/database") );
		}
		else if(m_ilist.type.at(i) == MCERCLE::PRODUCT) {
			item_Name -> setIcon( QIcon(":/app/products") );
		}
		else if(m_ilist.type.at(i) == MCERCLE::SERVICE) {
			item_Name -> setIcon( QIcon(":/app/services") );
		}
		item_TAX->setData(Qt::DisplayRole, m_ilist.tax.at(i));
		item_DISCOUNT->setData(Qt::DisplayRole, m_ilist.discount.at(i));
		item_PRICE->setData(Qt::DisplayRole, m_ilist.price.at(i));
		item_QUANTITY->setData(Qt::DisplayRole, m_ilist.quantity.at(i));

		//definir le tableau
		ui->tableWidget->setRowCount(i+1);

		//remplir les champs
		ui->tableWidget->setItem(i, COL_ID, item_ID);
		ui->tableWidget->setItem(i, COL_ID_PRODUCT, item_ID_PRODUCT);
		ui->tableWidget->setItem(i, COL_TYPE, item_TYPE);
		ui->tableWidget->setItem(i, COL_ORDER, item_ORDER);
		ui->tableWidget->setItem(i, COL_NAME, item_Name);
		ui->tableWidget->setCellWidget(i, COL_NAME, edit_name);
		ui->tableWidget->setItem(i, COL_TAX, item_TAX);
		ui->tableWidget->setItem(i, COL_DISCOUNT, item_DISCOUNT);
		ui->tableWidget->setItem(i, COL_PRICE, item_PRICE);
		ui->tableWidget->setItem(i, COL_QUANTITY, item_QUANTITY);
		
		// Applique la hauteur de la ligne en fonction des new lines
		ui->tableWidget->setRowHeight(i, (m_ilist.name.at(i).count("\n")+1) * 35); 
	}

	ui->tableWidget->setSortingEnabled(true);
	ui->tableWidget->sortItems(COL_ORDER, Qt::AscendingOrder);
	ui->tableWidget->selectRow(0);
}


/**
	Affiche les Services dans le tableau
*/
void DialogInvoice::listServiceToTable()
{
	service_common::serviceCommList servicesListCommon;
	service::serviceList userServicesList;

	//Clear les items, attention tjs utiliser la fonction clear()
	ui->tableWidget_selectService->clear();
	for (int i=ui->tableWidget_selectService->rowCount()-1; i >= 0; --i)
		ui->tableWidget_selectService->removeRow(i);
	for (int i=ui->tableWidget_selectService->columnCount()-1; i>=0; --i)
		ui->tableWidget_selectService->removeColumn(i);


	ui->tableWidget_selectService->setSortingEnabled(false);
	//Style de la table de l intervention
	ui->tableWidget_selectService->setColumnCount(SERV_COL_COUNT);
	ui->tableWidget_selectService->setColumnWidth(SERV_COL_NAME,225);
	ui->tableWidget_selectService->setColumnWidth(SERV_COL_PRICE,100);
	ui->tableWidget_selectService->setColumnWidth(SERV_COL_DETAIL,225);
	ui->tableWidget_selectService->setColumnHidden(SERV_COL_ID, true); //On cache la colonne des ID
	ui->tableWidget_selectService->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->tableWidget_selectService->setSelectionMode(QAbstractItemView::SingleSelection);
	ui->tableWidget_selectService->setEditTriggers(QAbstractItemView::NoEditTriggers);
	QStringList titles;
	titles << tr("Id") << tr("Nom") << tr("Prix") << tr("Tva") << QLatin1String("D�tail") ;
	if(!m_isTax){
		ui->tableWidget_selectService->setColumnHidden(SERV_COL_TAX , true); //cache la colonne TVA
	}
	
	ui->tableWidget_selectService->setHorizontalHeaderLabels( titles );

	//Recuperation des services commun avec ID = 0
	m_serviceComm->getServiceCommList(servicesListCommon, "NAME", "", "");
	//Recuperation des services utilisateur
	m_service->getServiceList(userServicesList, m_customer->getId(), "NAME", "", "");

	// list des services communs
	for(int i=0; i<servicesListCommon.id.size(); i++){
		ItemOfTable *item_ID_COM      = new ItemOfTable();
		ItemOfTable *item_NAME_COM    = new ItemOfTable();
		ItemOfTable *item_PRICE_COM   = new ItemOfTable();
		ItemOfTable *item_TAX_COM     = new ItemOfTable();
		ItemOfTable *item_Detail_COM  = new ItemOfTable();

		item_ID_COM ->		setData(Qt::DisplayRole, QString::number(servicesListCommon.id.at(i)));
		item_NAME_COM ->	setIcon( QIcon(":/app/services") );
		item_NAME_COM ->	setData(Qt::DisplayRole, servicesListCommon.name.at(i));
		item_PRICE_COM ->	setData(Qt::DisplayRole, servicesListCommon.price.at(i));
		item_TAX_COM ->		setData(Qt::DisplayRole, servicesListCommon.tax.at(i));
		item_Detail_COM ->	setData(Qt::DisplayRole, servicesListCommon.description.at(i));

		//definir le tableau
		ui->tableWidget_selectService->setRowCount(i+1);

		//remplir les champs
		ui->tableWidget_selectService->setItem(i, SERV_COL_ID, item_ID_COM);
		ui->tableWidget_selectService->setItem(i, SERV_COL_NAME, item_NAME_COM);
		ui->tableWidget_selectService->setItem(i, SERV_COL_PRICE, item_PRICE_COM);
		ui->tableWidget_selectService->setItem(i, SERV_COL_TAX, item_TAX_COM);
		ui->tableWidget_selectService->setItem(i, SERV_COL_DETAIL, item_Detail_COM);
	}

	// list des services utilisateur
	
	for(int i=0, j=ui->tableWidget_selectService->rowCount(); i<userServicesList.id.size(); i++){
		ItemOfTable *item_ID      = new ItemOfTable();
		ItemOfTable *item_NAME    = new ItemOfTable();
		ItemOfTable *item_PRICE   = new ItemOfTable();
		ItemOfTable *item_TAX   = new ItemOfTable();
		ItemOfTable *item_Detail  = new ItemOfTable();

		item_ID->setData(Qt::DisplayRole, QString::number(userServicesList.id.at(i)));
		item_NAME->setData(Qt::DisplayRole, userServicesList.name.at(i));
		item_PRICE->setData(Qt::DisplayRole, userServicesList.price.at(i));
		item_TAX->setData(Qt::DisplayRole, userServicesList.tax.at(i));
		item_Detail->setData(Qt::DisplayRole, userServicesList.description.at(i));
		
		//definir le tableau
		ui->tableWidget_selectService->insertRow( j );

		//remplir les champs
		ui->tableWidget_selectService->setItem(j, SERV_COL_ID, item_ID);
		ui->tableWidget_selectService->setItem(j, SERV_COL_NAME, item_NAME);
		ui->tableWidget_selectService->setItem(j, SERV_COL_PRICE, item_PRICE);
		ui->tableWidget_selectService->setItem(j, SERV_COL_TAX, item_TAX);
		ui->tableWidget_selectService->setItem(j, SERV_COL_DETAIL, item_Detail);
	}

	ui->tableWidget_selectService->setSortingEnabled(true);
}

/**
	Event de la selection d'un article dans la proposition
	pour charger son ID

  */
void DialogInvoice::on_tableWidget_itemSelectionChanged() {
	//Si plus d article on sort
	if(ui->tableWidget->rowCount()<=0)return;

	m_index_tabInvoice = ui->tableWidget->currentRow();
	//Si index < 0 on sort
	if(m_index_tabInvoice<0)return;
	m_articleInv_Id = ui->tableWidget->item(m_index_tabInvoice, 0)->text().toInt();
}

/**
 * @brief met a jour la taille des widget Edit BUG QT!
 */
void DialogInvoice::update_widgetSize() {
	//Si plus d article on sort
	if(ui->tableWidget->rowCount()<=0)return;
	
	QString name;
	for(int j=0; j <ui->tableWidget->rowCount(); j++) {
		// Applique la hauteur de la ligne en fonction des new lines
		if(ui->tableWidget->cellWidget(j, COL_NAME)){
			QTextEdit *get_name = qobject_cast<QTextEdit*>(ui->tableWidget->cellWidget(j, COL_NAME));
			name = get_name->toPlainText();
		}
		ui->tableWidget->setRowHeight(j, (name.count("\n")+1) * 35); 
	}
}

/**
 * @brief met a jour le champ ordre
 */
void DialogInvoice::update_OrderValue() {
	//Si plus d article on sort
	if(ui->tableWidget->rowCount()<=0)return;
	for(int i=0; i <ui->tableWidget->rowCount(); i++) {
		ui->tableWidget->item(i, COL_ORDER)->setData(Qt::DisplayRole, i);
	}
}

/**
	Elever l article selectionnee de la proposition
  */
void DialogInvoice::on_toolButton_rm_clicked() {
	//Si plus d article on sort
	if(ui->tableWidget->rowCount()<=0)
		return;
	QString mess = tr("Voulez-vous vraiment supprimer cet article<br><b>");
	if(ui->tableWidget->cellWidget(m_index_tabInvoice, COL_NAME)){
		mess += qobject_cast<QTextEdit*>(ui->tableWidget->cellWidget(m_index_tabInvoice, COL_NAME))->toPlainText();
	}
	mess += " </b>  ?";
	int ret = QMessageBox::warning(this, tr("Attention"),
									mess,
									QMessageBox::Yes, QMessageBox::No | QMessageBox::Default);

	if(ret == QMessageBox::Yes){
		ui -> tableWidget -> removeRow(m_index_tabInvoice);
		//Important selection litem 0 pour eviter des erreurs d index
		if(m_index_tabInvoice>=1) m_index_tabInvoice--;
		update_OrderValue();
		update_widgetSize();
		ui -> tableWidget -> selectRow(m_index_tabInvoice+1);
		emit(calcul_Total());
	}
}


/**
	Supprime les items de la base de donnees
  */
void DialogInvoice::removeProposalItems(){
	proposal::ProposalListItems ilist;
	bool present;
	/* Procedure pour savoir s'il faut supprimer des articles */
	m_proposal->getProposalItemsList(ilist, "ITEM_ORDER", "", "");
	for(int i=0; i<ilist.name.count(); i++){
		present = false;
		//On parcourt le tableau pour savoir si l ID et present
		for (int j=ui->tableWidget->rowCount()-1; j >= 0; --j){
			int idTable = ui->tableWidget->item(j, 0)->text().toInt();
			if(ilist.id.at(i) == idTable) present=true;
		}
		//S il nest pas present on le supprime de la base
		if(!present) m_proposal->removeProposalItem(ilist.id.at(i));
	}
}

/**
	Procedure pour mettre a jour les articles, ajoute et met a jour.
  */
void DialogInvoice::updateProposalItems(){
	proposal::ProposalItem itemInv;
	//On parcour le tableau pour mettre a jour les valeurs
	for (int j=ui->tableWidget->rowCount()-1; j >= 0; --j) {
		int id = ui->tableWidget->item(j, COL_ID)->text().toInt();
		itemInv.id = id;
		id = ui->tableWidget->item(j, COL_ID_PRODUCT)->text().toInt();
		itemInv.idProduct = id;

		if(ui->tableWidget->cellWidget(j, COL_NAME)){
			QTextEdit *get_name = qobject_cast<QTextEdit*>(ui->tableWidget->cellWidget(j, COL_NAME));
			itemInv.name = get_name->toPlainText();
		}
		itemInv.tax =  ui->tableWidget->item(j, COL_TAX)->text().toFloat();
		itemInv.discount = ui->tableWidget->item(j, COL_DISCOUNT)->text().toInt();
		itemInv.price = ui->tableWidget->item(j, COL_PRICE)->text().toFloat();
		itemInv.quantity = ui->tableWidget->item(j, COL_QUANTITY)->text().toInt();
		itemInv.type = ui->tableWidget->item(j, COL_TYPE)->text().toInt();
		itemInv.order = j;

		// si article present dans la bdd on modifi
		if(itemInv.id>-1) m_proposal->updateProposalItem( itemInv );
		//sinon on ajoute le produit
		else  m_proposal->addProposalItem( itemInv );
	}
}

/**
	Supprime les items de la base de donnees
  */
void DialogInvoice::removeInvoiceItems(){
	invoice::InvoiceListItems ilist;
	bool present;
	/* Procedure pour savoir s'il faut supprimer des articles */
	m_invoice->getInvoiceItemsList(ilist, "ITEM_ORDER", "", "");
	for(int i=0; i<ilist.name.count(); i++){
		present = false;
		//On parcourt le tableau pour savoir si l ID et present
		for (int j=ui->tableWidget->rowCount()-1; j >= 0; --j){
			int idTable = ui->tableWidget->item(j, 0)->text().toInt();
			if(ilist.id.at(i) == idTable) present=true;
		}
		//S il nest pas present on le supprime de la base
		if(!present) m_invoice->removeInvoiceItem(ilist.id.at(i));
	}
}

/**
	Procedure pour mettre a jour les articles, ajoute et met a jour.
  */
void DialogInvoice::updateInvoiceItems(){
	invoice::InvoiceItem itemInv;
	//On parcour le tableau pour mettre a jour les valeurs
	for (int j=ui->tableWidget->rowCount()-1; j >= 0; --j){
		int id = ui->tableWidget->item(j, COL_ID)->text().toInt();
		itemInv.id = id;
		id = ui->tableWidget->item(j, COL_ID_PRODUCT)->text().toInt();
		itemInv.idProduct = id;

		if(m_manageStock) {
			/// Si cest un produit gestion du stock automatise
			if(itemInv.idProduct > 0) {
				//Si le produit existe
				if(m_product->loadFromID( itemInv.idProduct )){
					int qte =0;
					//si cest une mise a jour on recupere la diff de quantite
					if(itemInv.id>-1) qte = getDiffQuantityOfItem( itemInv.id, ui->tableWidget->item(j, COL_QUANTITY)->text().toInt() );
					//sinon on recupere la qte de la cellule
					else qte = -ui->tableWidget->item(j, COL_QUANTITY)->text().toInt();
					m_product->setStock(m_product->getStock() + qte );
					m_product->update();
					if(m_product->getStock() < 0)
						QMessageBox::warning(this, tr("Attention"),
											 QLatin1String("Stock n�gatif pour le produit:\n") + m_product->getName() +" : "+QString::number( m_product->getStock() ) , QMessageBox::Ok);

				}
			}
		}

		if(ui->tableWidget->cellWidget(j, COL_NAME)){
			QTextEdit *get_name = qobject_cast<QTextEdit*>(ui->tableWidget->cellWidget(j, COL_NAME));
			itemInv.name = get_name->toPlainText();
		}
		itemInv.tax = ui->tableWidget->item(j, COL_TAX)->text().toFloat();
		itemInv.discount = ui->tableWidget->item(j, COL_DISCOUNT)->text().toInt();
		itemInv.price = ui->tableWidget->item(j, COL_PRICE)->text().toFloat();
		itemInv.quantity = ui->tableWidget->item(j, COL_QUANTITY)->text().toInt();
		if(!ui->tableWidget->item(j, COL_TYPE))
			itemInv.type = 0;
		else
			itemInv.type = ui->tableWidget->item(j, COL_TYPE)->text().toInt();
		itemInv.order = j;
		
		// si article present dans la bdd on modifie
		if(itemInv.id>-1) m_invoice->updateInvoiceItem( itemInv );
		//sinon on ajoute le produit
		else m_invoice->addInvoiceItem( itemInv );
	}
}
/**
	Recupere la difference de quantity entre la valeur initale et la valeur modifiee
  */
int DialogInvoice::getDiffQuantityOfItem(const int& id, int qteNew){
	int qteOld=0, qte=0;
	//on parcourt la liste pour avoir la quantite initiale
	for(int i=0; i< m_ilist.id.size(); i++)
		if(m_ilist.id.at(i) == id){
			qteOld = m_ilist.quantity.at(i);
			qte = qteOld - qteNew;
			return qte;
		}
	return 0;
}

/**
	Procedure pour mettre a jour les articles, ajoute et met a jour.
  */
void DialogInvoice::setProposal(unsigned char proc){
	m_proposal->setDescription( ui->lineEdit_description->text() );
	m_proposal->setUserDate( ui->dateEdit_DATE->date() );
	m_proposal->setState(ui->comboBox_State->currentIndex());
	m_proposal->setDeliveryDate(ui->dateEdit_delivery->date());
	m_proposal->setValidDate(ui->dateEdit_valid->date());

	QString typeP;
	switch(ui->comboBox_TYPE_PAYMENT->currentIndex()){
				case 0:typeP="";break;
				case 1:typeP=MCERCLE::CASH;break;
				case 2:typeP=MCERCLE::CHECK;break;
				case 3:typeP=MCERCLE::CREDIT_CARD;break;
				case 4:typeP=MCERCLE::INTERBANK;break;
				case 5:typeP=MCERCLE::TRANSFER;break;
				case 6:typeP=MCERCLE::DEBIT;break;
				case 7:typeP=MCERCLE::OTHER;break;
	}
	m_proposal->setTypePayment(typeP);
	m_proposal->setPrice( m_totalPrice );
	m_proposal->setPriceTax( m_totalTaxPrice );
	// suivant le process on modifi ou on ajoute la proposition commerciale
	if(proc == EDIT) m_proposal->update();
	else{
		//genere un nouveau code et cree lobjet
		m_proposal -> generateNewCode();
		if( m_proposal -> create() ) {
			createSuccess();
		}
		//recharge l objet et son nouvel ID pour lajout des items par la suite
		m_proposal->loadFromID(m_proposal->getLastId());
	}
}

/**
	Procedure pour mettre a jour les articles, ajoute et met a jour.
  */
void DialogInvoice::setInvoice(unsigned char proc){
	m_invoice->setDescription( ui->lineEdit_description->text() );
	m_invoice->setUserDate( ui->dateEdit_DATE->date() );
	m_invoice->setLimitPayment( ui->dateEdit_valid->date() );
	m_invoice->setPaymentDate( ui->dateEdit_delivery->date() );
	m_invoice->setState( ui->comboBox_State->currentIndex() );

	QString typeP;
	switch(ui->comboBox_TYPE_PAYMENT->currentIndex()){
				case 0:typeP="";break;
				case 1:typeP=MCERCLE::CASH;break;
				case 2:typeP=MCERCLE::CHECK;break;
				case 3:typeP=MCERCLE::CREDIT_CARD;break;
				case 4:typeP=MCERCLE::INTERBANK;break;
				case 5:typeP=MCERCLE::TRANSFER;break;
				case 6:typeP=MCERCLE::DEBIT;break;
				case 7:typeP=MCERCLE::OTHER;break;
	}
	m_invoice->setTypePayment(typeP);
	m_invoice->setPrice( m_totalPrice );
	m_invoice->setPriceTax( m_totalTaxPrice );
	// suivant le process on modifi ou on ajoute la facture/devis
	if(proc == EDIT) m_invoice->update();
	else{
		//On creer une nouvelle facture
		m_invoice->setType(MCERCLE::TYPE_INV);
		//cree lobjet avec un nouveau code
		m_invoice -> generateNewCode();
		if( m_invoice -> create() ) {
			createSuccess();
		}
		//recharge l objet et son nouvel ID pour lajout des items par la suite
		m_invoice -> loadFromID(m_invoice->getLastId());
	}
}

/**
 * @brief DialogInvoice::createSuccess
 */
void DialogInvoice::createSuccess() {
	QMessageBox::information(this, tr("Information"), QLatin1String("Cr�ation r�alis�e"), QMessageBox::Ok);
	loadValues();
	ui -> pushButton_print -> setEnabled( true );
	ui -> pushButton_ok -> setEnabled( false );
}

/**
	Applique les changements au devis/facture
  */
void DialogInvoice::apply() {
	/// Si on est en mode Edition
	if(m_DialogState == EDIT){
		QString mess;
		if(m_DialogType == PROPOSAL_TYPE) mess = tr("Voulez-vous vraiment modifier le devis ?");
		else mess = tr("Voulez-vous vraiment modifier la facture ?");
		int ret = QMessageBox::warning(this, tr("Attention"), mess, QMessageBox::Yes, QMessageBox::No | QMessageBox::Default);
		if(ret == QMessageBox::Yes){
			// modification des articles
			if(m_DialogType == PROPOSAL_TYPE){
				removeProposalItems();
				updateProposalItems();
				setProposal(m_DialogState);
				//Demande le rafraichissement des listes
				emit askRefreshProposalList();
				// Active ou desactive le bouton de creation de facture depuis devis
				if( (m_proposal->getState() ==  MCERCLE::PROPOSAL_VALIDATED) && (m_proposal->getInvoiceCode().isEmpty()) ){
					ui->pushButton_createInv->setEnabled( true );
				}
				else{
					ui->pushButton_createInv->setEnabled( false );
				}
			}
			else{
				removeInvoiceItems();
				updateInvoiceItems();
				setInvoice(m_DialogState);
				//Demande le rafraichissement des listes
				emit askRefreshInvoiceList();
			}
		}//End modif
	}

	/// Sinon en mode creation
	else{
		if(m_DialogType == PROPOSAL_TYPE){
			// Ajoute la proposition
			setProposal(m_DialogState);
			// Ajout des articles
			updateProposalItems();
			//Demande le rafraichissement des listes
			emit askRefreshProposalList();
		}
		else{
			// Ajoute la facture
			setInvoice(m_DialogState);
			// Ajout des articles
			updateInvoiceItems();
			//Demande le rafraichissement des listes
			emit askRefreshInvoiceList();
		}
		this->close();
	}
}


/**
  Ajout/Modification de la proposition
*/
void DialogInvoice::on_pushButton_ok_clicked() {
	apply();
}



/**
 * @brief DialogInvoice::on_pushButton_close_clicked
 */
void DialogInvoice::on_pushButton_close_clicked() {
	this->close();
}

/**
	Sur ledition des cellules on calcule le total
*/
 void DialogInvoice::on_tableWidget_cellChanged(int row, int column) {
if((column == COL_PRICE) || (column == COL_TAX) || (column == COL_QUANTITY) || (column == COL_DISCOUNT)){
		//SECURITE LORS DES DEPLACEMENTS
		//Car cela vide la cellule de sont objet "ItemOfTable" donc -> SIGFAULT
		
		//Test pour savoir si les colonnes sont renseignees
		if((ui->tableWidget->item(row, COL_PRICE) == NULL)||
			(ui->tableWidget->item(row, COL_QUANTITY) == NULL)||
			(ui->tableWidget->item(row, COL_DISCOUNT) == NULL))return;

		qreal priceU = ui->tableWidget->item(row, COL_PRICE)->text().toFloat();
		//qreal Tax = ui->tableWidget->item(row, COL_TAX)->text().toFloat();
		int quantity = ui->tableWidget->item(row, COL_QUANTITY)->text().toInt();
		int discount = ui->tableWidget->item(row, COL_DISCOUNT)->text().toInt();
		//Calcul du total
		qreal price = priceU * quantity;
		price -= price * (discount/100.0);
		//Tronc a 2 decimal
		price = QString::number(price,'f',2).toFloat();
		//Item du total
		ItemOfTable *item_Total  = new ItemOfTable();
		item_Total->setData(Qt::DisplayRole, price);
		item_Total->setFlags(item_Total->flags() & (~Qt::ItemIsEditable)); // Ne pas rendre editable
		ui->tableWidget->setItem(row, COL_TOTAL, item_Total);

		emit(calcul_Total());
	}
}


/**
  Calcul et Affiche le total
*/
void DialogInvoice::calcul_Total() {
	m_totalPrice = m_totalTaxPrice = 0.00;
	//On parcour le tableau pour savoir si l ID et present
	for (int j=ui->tableWidget->rowCount()-1; j >= 0; --j){
		if( (ui->tableWidget->item(j, COL_TOTAL)	!= NULL) &&
			 (ui->tableWidget->item(j, COL_TAX)		!= NULL)){
			m_totalPrice += ui->tableWidget->item(j, COL_TOTAL)->text().toDouble();
			m_totalTaxPrice += ui->tableWidget->item(j, COL_TOTAL)->text().toDouble() + ((ui->tableWidget->item(j, COL_TOTAL)->text().toDouble()*ui->tableWidget->item(j, COL_TAX)->text().toDouble())/100.0);
		}
	}
	qDebug() << "m_totalPrice=" << m_totalPrice;
	qDebug() << "m_totalTaxPrice=" << m_totalTaxPrice;


	//affiche la valeur
	QString val;
	if(!m_isTax){
			val = "<div class=\"ac\" style=\"background: #B0E9AE;\"><strong>"+ tr("TOTAL: ") + m_lang.toString(m_totalPrice,'f',2) + tr(" &euro; </strong></div>");
	}
	else{
			val = "<div class=\"ac\" style=\"background: #B0E9AE;\"><strong>"+ tr("TOTAL HT: ") + m_lang.toString(m_totalPrice,'f',2) + tr(" &euro; </strong></div>");
						val += "<strong>"+ tr("TOTAL TVA: ") + m_lang.toString(m_totalTaxPrice-m_totalPrice,'f',2) + tr(" &euro; </strong><br>");
			val += "<strong>"+ tr("TOTAL TTC: ") + m_lang.toString(m_totalTaxPrice,'f',2) + tr(" &euro; </strong><br>");
	}

	if(m_DialogType == INVOICE_TYPE){
		//Cacul du reste Total HT - acompte
		qreal diff=0;
		if(!m_isTax){
			diff = m_totalPrice - m_invoice->getPartPayment();
			val += "<strong>"+ tr("ACOMPTE: ") + m_lang.toString(m_invoice->getPartPayment(),'f',2) + tr(" &euro; </strong><br>");
		}
		else {
			diff = m_totalTaxPrice - m_invoice->getPartPaymentTax();
			val += "<strong>"+ tr("ACOMPTE: ") + m_lang.toString(m_invoice->getPartPaymentTax(),'f',2) + tr(" &euro; </strong><br>");
		}
		val += "<strong>"+ tr("RESTE A PAYER: ") + m_lang.toString(diff,'f',2) + tr(" &euro; </strong>");
	}
	else {
		val += "<strong>"+ tr("ACOMPTE DE 30%: ") + m_lang.toString(m_totalTaxPrice*0.3,'f',2) + tr(" &euro; </strong>");
	}
	ui->label_Total->setText( val );
}

/**
  Ajout dun article dans le tableau proposition
  */
void DialogInvoice::add_to_Table(int typeITEM, int idProduct, QString name, qreal mtax, qreal price) {
	int cRow = ui->tableWidget->rowCount();
	
	//Si pas de ligne on cree les colonnes
	if(cRow<=0){
		//Style de la table de proposition
		ui->tableWidget->setColumnCount(COL_COUNT);
		ui->tableWidget->setColumnWidth(COL_NAME, 375);
	#ifdef QT_NO_DEBUG
		ui->tableWidget->setColumnHidden(COL_ID, true); //cache la colonne ID ou DEBUG
		ui->tableWidget->setColumnHidden(COL_ID_PRODUCT, true); //cache la colonne ID ou DEBUG
		ui->tableWidget->setColumnHidden(COL_ORDER , true); //cache la order ou DEBUG
		ui->tableWidget->setColumnHidden(COL_TYPE , true); //cache la order ou DEBUG
	#endif

		QStringList titles;
		titles  << tr("Id") << tr("Id Produit") << tr("Type") << tr("Ordre") << tr("Nom") << tr("Tva") << tr("Remise(%)") << tr("Prix") << QLatin1String("Quantit�");
		if(!m_isTax){
				titles << tr("Total");
				ui->tableWidget->setColumnHidden(COL_TAX , true); //cache la colonne TVA
		}
		else	titles << tr("Total HT");
		ui->tableWidget->setHorizontalHeaderLabels( titles );

		if(!m_isTax) ui->tableWidget->setColumnHidden(COL_TAX , true); //cache la colonne TVA
	}
	// on increment la quantite si article deja present
	else{
		for (int j=cRow-1; j >= 0; --j){
			if(ui->tableWidget->cellWidget(j, COL_NAME)) {
				if( (name  == qobject_cast<QTextEdit*>(ui->tableWidget->cellWidget(j, COL_NAME))->toPlainText()) &&
					(price == ui->tableWidget->item(j, COL_PRICE)->text().toFloat()) ) {
					ItemOfTable *item_QUANT  = new ItemOfTable();
					item_QUANT->setData(Qt::DisplayRole, ui->tableWidget->item(j, COL_QUANTITY)->text().toInt()+1);
					ui->tableWidget->setItem(j,COL_QUANTITY, item_QUANT);
					return; //<<-- on sort
				}
			}
		}
	}
	// Sinon on creer une nouvelle ligne
	ItemOfTable *item_ID           = new ItemOfTable();
	ItemOfTable *item_ID_PRODUCT   = new ItemOfTable();
	ItemOfTable *item_TYPE         = new ItemOfTable();
	ItemOfTable *item_ORDER        = new ItemOfTable();
	ItemOfTable *item_Name         = new ItemOfTable();
	ItemOfTable *item_TAX          = new ItemOfTable();
	ItemOfTable *item_DISCOUNT     = new ItemOfTable();
	ItemOfTable *item_PRICE        = new ItemOfTable();
	ItemOfTable *item_QUANTITY     = new ItemOfTable();

	item_ID->setData(Qt::DisplayRole, -1);
	item_ID->setFlags(item_ID->flags() & (~Qt::ItemIsEditable)); // Ne pas rendre editable
	item_ID_PRODUCT->setData(Qt::DisplayRole, idProduct);
	item_ID_PRODUCT->setFlags(item_ID_PRODUCT->flags() & (~Qt::ItemIsEditable)); // Ne pas rendre editable
	item_TYPE->setData(Qt::DisplayRole, typeITEM);
	item_ORDER->setData(Qt::DisplayRole, cRow);
	item_ORDER->setFlags(item_ORDER->flags() & (~Qt::ItemIsEditable)); // Ne pas rendre editable

	QTextEdit *edit_name = new QTextEdit();
	edit_name ->setText( name );
	// Ne pas rendre editable si c est un produit! + Ajout d'image
	// Car il il y a une liaison pour la gestion du stock
	if(idProduct > 0){
		edit_name -> setReadOnly(true);
		item_Name -> setIcon( QIcon(":/app/database") );
	}
	else if(typeITEM == MCERCLE::PRODUCT){
		item_Name -> setIcon( QIcon(":/app/products") );
	}
	else if(typeITEM == MCERCLE::SERVICE){
		item_Name -> setIcon( QIcon(":/app/services") );
	}
	item_PRICE->setData		(Qt::DisplayRole, price);
	item_TAX->setData		(Qt::DisplayRole, mtax);
	item_QUANTITY->setData	(Qt::DisplayRole, 1);
	item_DISCOUNT->setData	(Qt::DisplayRole, 0);

	//definir le tableau
	ui->tableWidget->setRowCount(cRow+1);

	//remplir les champs
	ui->tableWidget->setItem(cRow, COL_ID, item_ID);
	ui->tableWidget->setItem(cRow, COL_ID_PRODUCT, item_ID_PRODUCT);
	ui->tableWidget->setItem(cRow, COL_TYPE, item_TYPE);
	ui->tableWidget->setItem(cRow, COL_ORDER, item_ORDER);
	ui->tableWidget->setItem(cRow, COL_NAME, item_Name);
	ui->tableWidget->setCellWidget(cRow, COL_NAME, edit_name);
	ui->tableWidget->setItem(cRow, COL_TAX, item_TAX);
	ui->tableWidget->setItem(cRow, COL_DISCOUNT, item_DISCOUNT);
	ui->tableWidget->setItem(cRow, COL_PRICE, item_PRICE);
	ui->tableWidget->setItem(cRow, COL_QUANTITY, item_QUANTITY);
	
	// Applique la hauteur de la ligne en fonction des new lines
	ui->tableWidget->setRowHeight(cRow, (edit_name->toPlainText().count("\n")+1) * 30);
	ui->tableWidget->selectRow(cRow+1);
}


/**
  Ajout dun article selectionne
  */
void DialogInvoice::on_toolButton_add_clicked() {
	//si c est un service
	if(ui->tabWidget_select->currentIndex() == 0){
		//Si plus d article on sort
		if(ui->tableWidget_selectService->rowCount()<=0)return;

		int m_index_tabService = ui->tableWidget_selectService->currentRow();
		//Si index < 0 on sort
		if(m_index_tabService<0)return;
		QString text = ui->tableWidget_selectService->item(m_index_tabService, SERV_COL_NAME)->text() + "\n";
		text += ui->tableWidget_selectService->item(m_index_tabService, SERV_COL_DETAIL)->text();
		add_to_Table(MCERCLE::SERVICE, 0, text, ui->tableWidget_selectService->item(m_index_tabService, SERV_COL_TAX)->text().toDouble(),
					ui->tableWidget_selectService->item(m_index_tabService, SERV_COL_PRICE)->text().toDouble() );
	}
	//ou un produit
	else{
		//Si plus d article on sort
		if(m_productView->getRowCount()<=0)return;
		//Ajoute a la table, mais ne pas rendre editable car le produit est lier pour le stock automatique
		add_to_Table(MCERCLE::PRODUCT, m_productView->getSelectedProductID(), m_productView->getSelectedProductName(),
					  m_productView->getSelectedProductTax(), m_productView->getSelectedProductPrice() );
	}
}


/**
	Affiche le menu pour ajouter une ligne dedition libre
  */
void DialogInvoice::on_toolButton_addFreeline_clicked() {
	ui->toolButton_addFreeline->showMenu();
}

/**
  Ajoute une ligne dedition libre comptabilisee en service
  */
void DialogInvoice::addFreeline_Service() {
	add_to_Table(MCERCLE::SERVICE, 0, "Service", 0.0,0 );
}

/**
  Ajoute une ligne dedition libre comptabilisee en produit
  */
void DialogInvoice::addFreeline_Product() {
	add_to_Table(MCERCLE::PRODUCT, 0, "Produit", 0.0,0 );
}


/**
	Creation dune facture associee
  */
void DialogInvoice::on_pushButton_createInv_clicked()
{
	int ret = QMessageBox::warning(this, tr("Attention"), QLatin1String("Voulez-vous vraiment cr�er une facture depuis le devis ?") , QMessageBox::Yes, QMessageBox::No | QMessageBox::Default);
	if(ret == QMessageBox::Yes){
		createInvoiceFromProposal();
		this->close();
	}
}


/**
	Creer une facture associee a la proposition
*/
void DialogInvoice::createInvoiceFromProposal() {
	m_invoice->setIdCustomer( m_proposal->getIdCustomer() );
	m_invoice->setDescription( m_proposal->getDescription() );
	m_invoice->setState( MCERCLE::INV_UNPAID );
	m_invoice->setUserDate( QDate::currentDate() );
	m_invoice->setPaymentDate( QDate::currentDate() );
	m_invoice->setLimitPayment( QDate::currentDate() );

	m_invoice->setTypePayment( m_proposal->getTypePayment() );
	m_invoice->setPrice( m_proposal->getPrice() );
	qDebug() << "m_proposal->getPriceTax():" << m_proposal->getPriceTax();
	m_invoice->setPriceTax( m_proposal->getPriceTax() );
	m_invoice->setType(MCERCLE::TYPE_INV);
	m_invoice->setIdRef(0);
	// creation de la valeur de lacompte
	m_invoice->setPartPayment( 0 );
	m_invoice->setPartPaymentTax( 0 );


	//cree lobjet avec un nouveau code
	m_invoice -> generateNewCode();
	m_invoice -> create();
	//recharge l objet et son nouvel ID pour lajout des items par la suite
	m_invoice -> loadFromID(m_invoice->getLastId());

	//Creation des items !
	proposal::ProposalListItems itemPro;
	invoice::InvoiceItem itemInv;
	//Recuperation des donnees presentent dans la bdd
	m_proposal->getProposalItemsList(itemPro, "ITEM_ORDER", "", "");
	for(int i=0; i<itemPro.name.count(); i++){
		itemInv.name = itemPro.name.at(i);
		itemInv.idProduct = itemPro.idProduct.at(i);
		itemInv.price = itemPro.price.at(i);
		itemInv.discount = itemPro.discount.at(i);
		itemInv.tax = itemPro.tax.at(i);
		itemInv.quantity = itemPro.quantity.at(i);
		itemInv.order = itemPro.order.at(i);
		itemInv.type = itemPro.type.at(i);
		
		if(m_manageStock){
			/// Si cest un produit gestion du stock automatise
			if(itemInv.idProduct > 0) {
				//Si le produit existe
				if(m_product->loadFromID( itemInv.idProduct )){
					m_product->setStock(m_product->getStock() - itemInv.quantity );
					m_product->update();
					if(m_product->getStock() < 0)
						QMessageBox::warning(this, tr("Attention"),
											 QLatin1String("Stock n�gatif pour le produit:\n") + m_product->getName() +" : "+QString::number( m_product->getStock() ) , QMessageBox::Ok);

				}
			}
		}
		m_invoice->addInvoiceItem(itemInv);
	}
	//Creer le lien entre la proposition et la facture
	m_proposal->setLink(m_proposal->getId(), m_invoice->getId() );

	//Demande le rafraichissement de la liste des factures
	emit askRefreshInvoiceList();
}

 
/**
	Monter les articles dans la liste
  */
void DialogInvoice::on_toolButton_up_clicked(){
	//Si on est en tete de liste, on sort
	int index = ui->tableWidget->currentRow();
	if(index <= 0)return;
	//Sinon on monte l'article
	else{
		/// Deplacement de ligne
		/// Pas prevue par l API d origine :-/
		// Recupere 2 lignes pour permutter
		QList<QTableWidgetItem*>  rowItemsToUp, rowItemsToDn;
		QList<QTextEdit*> rowEditToUp, rowEditToDn;

		// Attention ne pas recupere la colonne derniere col TOTAL qui est inseree en auto par une autre fonction!!!
		// d ou le ui->tableWidget->columnCount()-1
		for (int col = 0; col < ui->tableWidget->columnCount()-1; ++col) {
			rowItemsToUp << ui->tableWidget->takeItem(index, col);
			rowItemsToDn << ui->tableWidget->takeItem(index-1, col);
			
			if( col == COL_NAME){
				QTextEdit *EtoUp = new QTextEdit();
				EtoUp -> setText( qobject_cast<QTextEdit*>( ui->tableWidget->cellWidget(index, col))->toPlainText() );
				EtoUp -> setReadOnly( qobject_cast<QTextEdit*>( ui->tableWidget->cellWidget(index, col))->isReadOnly() );
				
				QTextEdit *EtoDn = new QTextEdit();
				EtoDn -> setText( qobject_cast<QTextEdit*>( ui->tableWidget->cellWidget(index-1, col))->toPlainText() );
				EtoDn -> setReadOnly( qobject_cast<QTextEdit*>( ui->tableWidget->cellWidget(index-1, col))->isReadOnly() );

				rowEditToUp << EtoUp;
				rowEditToDn << EtoDn;
			}
		}

		// Change les lignes
		for (int colu = 0, item=0, widg=0; colu < ui->tableWidget->columnCount()-1; ++colu) {
			ui->tableWidget->removeCellWidget(index-1, colu);
			ui->tableWidget->removeCellWidget(index, colu);
			
			ui->tableWidget->setItem(index-1, colu, rowItemsToUp.at(item));
			ui->tableWidget->setItem(index, colu, rowItemsToDn.at(item++));
			
			if( colu == COL_NAME){
				
				ui->tableWidget->setCellWidget(index-1, colu, rowEditToUp.at(widg));
				
				ui->tableWidget->setCellWidget(index, colu, rowEditToDn.at(widg++));
			}
		}
		// Reselection ligne en question
		ui->tableWidget->selectRow(index-1);
	}
}

/**
	Descendre les articles dans la liste
  */
void DialogInvoice::on_toolButton_dn_clicked()
{
	//Si on est en tete de liste, on sort
	int index = ui->tableWidget->currentRow();
	if(index >= ui->tableWidget->rowCount()-1)return;
	//Sinon on monte l'article
	else{
		/// Deplacement de ligne
		/// Pas prevue par l API d origine :-/
		// Recupere 2 lignes pour permutter
		QList<QTableWidgetItem*>  rowItemsToUp, rowItemsToDn;
		QList<QTextEdit*> rowEditToUp, rowEditToDn;
		
		// Attention ne pas recupere la colonne derniere col TOTAL qui est inseree en auto par une autre fonction!!!
		// d ou le ui->tableWidget->columnCount()-1
		for (int col = 0; col < ui->tableWidget->columnCount()-1; ++col) {
			if( col == COL_NAME){
				QTextEdit *EtoUp = new QTextEdit();
				EtoUp -> setText( qobject_cast<QTextEdit*>( ui->tableWidget->cellWidget(index+1, col))->toPlainText() );
				EtoUp -> setReadOnly( qobject_cast<QTextEdit*>( ui->tableWidget->cellWidget(index+1, col))->isReadOnly() );
				
				QTextEdit *EtoDn = new QTextEdit();
				EtoDn -> setText( qobject_cast<QTextEdit*>( ui->tableWidget->cellWidget(index, col))->toPlainText() );
				EtoDn -> setReadOnly( qobject_cast<QTextEdit*>( ui->tableWidget->cellWidget(index, col))->isReadOnly() );

				rowEditToUp << EtoUp;
				rowEditToDn << EtoDn;
			}
			else{
				rowItemsToUp << ui->tableWidget->takeItem(index+1, col);
				rowItemsToDn << ui->tableWidget->takeItem(index, col);
			}
		}

		// Change les lignes
		for (int colu = 0, item=0, widg=0; colu < ui->tableWidget->columnCount()-1; ++colu) {
			if( colu == COL_NAME){
				ui->tableWidget->removeCellWidget(index, colu);
				ui->tableWidget->setCellWidget(index, colu, rowEditToUp.at(widg));
				ui->tableWidget->removeCellWidget(index+1, colu);
				ui->tableWidget->setCellWidget(index+1, colu, rowEditToDn.at(widg++));
			}
			else{
				ui->tableWidget->setItem(index, colu, rowItemsToUp.at(item));
				ui->tableWidget->setItem(index+1, colu, rowItemsToDn.at(item++));
			}
		}
		// Reselection ligne en question
		ui->tableWidget->selectRow(index+1);
	}
}

/**
 * @brief DialogInvoice::on_pushButton_print_clicked
 */
void DialogInvoice::on_pushButton_print_clicked() {
	// Si c est un devis
	if(m_DialogType == PROPOSAL_TYPE){
		Printc print(m_data, m_lang);
		print.print_Proposal( m_proposal -> getId() );
	}
	// ou une facture
	else{
		Printc print(m_data, m_lang);
		print.print_Invoice( m_invoice -> getId() );
	}
}


/**
 * @brief Affiche/cache la base de donnee
 */
void DialogInvoice::on_toolButton_hide_clicked() {
	ui->groupBox_select->setVisible( !ui->groupBox_select->isVisible() );
	ui->toolButton_add->setVisible( !ui->toolButton_add->isVisible() );
}

/**
 * @brief creer une facture dacompte
 */
void DialogInvoice::on_pushButton_partInvoice_clicked(int type) {
	qDebug() << "type:" << type;
	int ret = QMessageBox::information(this, tr("Attention"),
				QLatin1String("Voulez-vous cr�er une facture d'acompte? "),
				QMessageBox::Yes, QMessageBox::No | QMessageBox::Default);

	if(ret == QMessageBox::Yes){
		// Avertissement si un acompte existe d�ja
		if(m_invoice->isTypeExiste( MCERCLE::TYPE_PART )) {
			ret = QMessageBox::warning(this, tr("Attention"),
							QLatin1String("Un acompte existe d�ja sur cette facture\n\nVoulez-vous en cr�er un nouveau? "),
							QMessageBox::Yes, QMessageBox::No | QMessageBox::Default);

			if(ret == QMessageBox::No){
				return;
			}
		}

		//Choix de la tax
		qreal mtax;
		if(m_data->getIsTax()) {
			DialogTax *m_DialogTax = new DialogTax(m_data->m_tax, MCERCLE::Choice, &mtax);
			m_DialogTax->setModal(true);
			m_DialogTax->exec();
		}
		QString desc = m_invoice->getDescription();
		m_invoice->setType( MCERCLE::TYPE_PART );
		m_invoice->setIdRef( m_invoice->getId() );
		m_invoice->setDescription( tr("Acompte sur ")+ m_invoice->getCode() );
		m_invoice->setState( MCERCLE::INV_UNPAID );
		m_invoice->setUserDate( QDate::currentDate() );
		m_invoice->setPaymentDate( QDate::currentDate() );
		m_invoice->setLimitPayment( QDate::currentDate() );
		m_invoice->setTypePayment( m_invoice->getTypePayment() );
		m_invoice->setPrice( m_totalTaxPrice*0.3 / ( 1+ mtax/100.0) );
		m_invoice->setPriceTax( m_totalTaxPrice*0.3);/* 30% acompte */
		m_invoice->setPartPayment(0);
		m_invoice->setPartPaymentTax(0);

		//cree lobjet avec un nouveau code
		m_invoice -> generateNewCode();
		m_invoice -> create();
		//recharge l objet et son nouvel ID pour lajout des items par la suite
		m_invoice -> loadFromID(m_invoice->getLastId());

		//Creation dun item !
		invoice::InvoiceItem itemInv;
		itemInv.name = tr("Acompte sur ")+ desc;
		itemInv.idProduct = 0;
		itemInv.price = m_invoice->getPrice();
		itemInv.discount = 0;
		itemInv.tax = mtax;
		itemInv.quantity = 1;
		itemInv.order = 0;
		itemInv.type = type;

		m_invoice->addInvoiceItem(itemInv);

		//Configuration de l'UI
		setUI();
		//Demande le rafraichissement de la liste des factures
		emit askRefreshInvoiceList();
	}
}

/**
 * @brief Creer une facture davoir
 */
void DialogInvoice::on_pushButton_creditInvoice_clicked() {

/*	int ret = QMessageBox::information(this, tr("Attention"),
				QLatin1String("Voulez-vous cr�er une facture d'avoir? "),
				QMessageBox::Yes, QMessageBox::No | QMessageBox::Default);

	if(ret == QMessageBox::Yes){
		// Avertissement si un acompte existe d�ja
		if(m_invoice->isTypeExiste( MCERCLE::TYPE_CREDIT )) {
			ret = QMessageBox::warning(this, tr("Attention"),
							QLatin1String("Un avoir existe d�ja sur cette facture\n\nVoulez-vous en cr�er un nouveau? "),
							QMessageBox::Yes, QMessageBox::No | QMessageBox::Default);

			if(ret == QMessageBox::No){
				return;
			}
		}

		m_invoice->setType( MCERCLE::TYPE_CREDIT );
		m_invoice->setIdRef( m_invoice->getId() );
		m_invoice->setDescription( tr("Avoir sur ")+ m_invoice->getCode() );
		m_invoice->setState( MCERCLE::INV_UNPAID );
		m_invoice->setUserDate( QDate::currentDate() );
		m_invoice->setPaymentDate( QDate::currentDate() );
		m_invoice->setLimitPayment( QDate::currentDate() );
		m_invoice->setTypePayment( m_invoice->getTypePayment() );
		m_invoice->setPrice( -m_invoice->getPrice() );
		m_invoice->setPriceTax( -m_invoice->getPriceTax() );
		m_invoice->setPartPayment(0);

		//Recuperation des items presentent dans la factures
		invoice::InvoiceListItems itemsInv;
		m_invoice->getInvoiceItemsList(itemsInv, "ITEM_ORDER", "", "");
		//cree lobjet avec un nouveau code
		m_invoice -> generateNewCode();
		m_invoice -> create();

		//Creation des items !
		invoice::InvoiceItem itemInv;
		for(int i=0; i<itemsInv.name.count(); i++){
			itemInv.name = itemsInv.name.at(i);
			itemInv.idProduct = itemsInv.idProduct.at(i);
			itemInv.price = (- itemsInv.price.at(i) );
			itemInv.discount = itemsInv.discount.at(i);
			itemInv.tax = itemsInv.tax.at(i);
			itemInv.quantity = itemsInv.quantity.at(i);
			itemInv.order = itemsInv.order.at(i);
			itemInv.type = itemsInv.type.at(i);

			m_invoice->addInvoiceItem(itemInv);
		}

		//Configuration de l'UI
		setUI();
		//Demande le rafraichissement de la liste des factures
		emit askRefreshInvoiceList();
	}*/
}
