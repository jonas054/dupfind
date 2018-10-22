class Cat extends Component {
  // Allow only: A-Ö a-ö 0-9 & . : ; - + / * = > < ( ) %
  ALPHA_NUMERIC_REGEX = /^[A-ZÅÄÖa-zåäö0-9\s&.:;\-+/*=><()%]*$/g;
  // Allow the following characters: A-Ö a-ö 0-9 ! - + % " / ? , . §
  REFERENCES_REGEX = /^[A-ZÅÄÖa-zåäö0-9\s!\-+%"/?,.§]*$/g;

  constructor(props) {
    super(props);
    const { kettleData } = props;

    this.state = this.clearState(kettleData.referenceId);

    this.storeTransaction = this.storeTransaction.bind(this);
    this.onChange = this.onChange.bind(this);
    this.mapStateToTransactionType = this.mapStateToTransactionType.bind(this);
    this.checkRequiredFields = this.checkRequiredFields.bind(this);
    props.setTransactionIsDirty(false);
  }

  // Allow only: A-Ö a-ö 0-9 & . : ; - + / * = > < ( ) %
  ALPHA_NUMERIC_REGEX = /^[A-ZÅÄÖa-zåäö0-9\s&.:;\-+/*=><()%]*$/g;

  constructor(props) {
    super(props);
    const { kettleData } = props;

    this.state = this.clearState(kettleData.referenceId);

    this.storeTransaction = this.storeTransaction.bind(this);
    this.onChange = this.onChange.bind(this);
    this.mapStateToTransactionType = this.mapStateToTransactionType.bind(this);
    this.checkRequiredFields = this.checkRequiredFields.bind(this);
    props.setTransactionIsDirty(false);
  }
}
